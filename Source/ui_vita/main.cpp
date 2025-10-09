#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <psp2/appmgr.h>
#include <psp2/ctrl.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/sysmem.h>
#include "debug_screen.h"
#include "file_chooser.h"

#include "Log.h"
#include "AppConfig.h"
#include "PS2VM.h"
#include "PH_Generic.h"
#include "gs/GSH_GXM/GSH_GXM.h"

#include "PS2VM_Preferences.h"

#include "PathUtils.h"
#include "PtrStream.h"
#include "MemStream.h"

#define PLAY_PATH	"ux0:/Play"
#define DEFAULT_FILE 	"ux0:/Play/cube.elf"

#define EXIT_MASK (SCE_CTRL_CROSS | SCE_CTRL_CIRCLE)

// static int cy = 0;

#define LOG(...) \
	do { \
		printf(__VA_ARGS__); \
		/*debug_screen_draw_stringf(10, cy+=16, COLOR_WHITE, __VA_ARGS__);*/ \
	} while (0)

extern "C" {

int _newlib_heap_size_user = 164 * 1024 * 1024;

void __assert_func(const char *filename, int line, const char *assert_func, const char *expr)
{
	LOG("%s:%d:4: error: assertion \"%s\" failed in function %s\n",
		filename, line, expr, assert_func );
	abort();
}

void abort(void)
{
	LOG("Abort called\n");
	sceKernelExitProcess(0);
	while (1) sleep(1);
}

}

static void wait_press()
{
	SceCtrlData pad;
	memset(&pad, 0, sizeof(pad));
	while (1) {
		sceCtrlReadBufferPositive(0, &pad, 1);

		if (pad.buttons & SCE_CTRL_CROSS)
			break;

	}
}

#define RWX_BLOCK_SIZE	(16 * 1024 * 1024)
static SceUID rwx_block_uid = -1;
static void *rwx_block_addr = 0;
static uint32_t rwx_block_head = 0;
static SceKernelLwMutexWork rwx_block_mutex;

static void vita_rwx_block_create()
{
	rwx_block_uid = sceKernelAllocMemBlockForVM("Play_Vita_rwx_block", RWX_BLOCK_SIZE);
	sceKernelGetMemBlockBase(rwx_block_uid, &rwx_block_addr);
	sceKernelCreateLwMutex(&rwx_block_mutex, "Play_Vita_rwx_block_mutex", 0, 0, NULL);
}

static void vita_rwx_block_free()
{
	sceKernelDeleteLwMutex(&rwx_block_mutex);
	sceKernelFreeMemBlock(rwx_block_uid);
}

void *vita_rwx_alloc(uint32_t size)
{
	sceKernelLockLwMutex(&rwx_block_mutex, 1, NULL);

	assert((rwx_block_head + size) <= RWX_BLOCK_SIZE);
	void *head = (void *)((uintptr_t)rwx_block_addr + rwx_block_head);
	rwx_block_head += size;

	sceKernelUnlockLwMutex(&rwx_block_mutex, 1);
	return head;
}

int vita_rwx_sync(void *addr, uint32_t size)
{
	return sceKernelSyncVMDomain(rwx_block_uid, addr, size);
}

static bool IsBootableExecutablePath(const fs::path& filePath)
{
	auto extension = filePath.extension().string();
	std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
	return (extension == ".elf");
}

static bool IsBootableDiscImagePath(const fs::path& filePath)
{
	auto extension = filePath.extension().string();
	std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
	return (extension == ".iso") ||
	       (extension == ".isz") ||
	       (extension == ".cso") ||
	       (extension == ".bin");
}

static void start_emu(const char *file, bool &executionOver)
{
	SceCtrlData pad;
	CPS2VM *m_virtualMachine = nullptr;
	fs::path filePath(file);

	Framework::PathUtils::SetFilesDirPath(PLAY_PATH);
	Framework::PathUtils::SetCacheDirPath(PLAY_PATH);

	CAppConfig::GetInstance().SetPreferenceBoolean("log.showprints", true);

	LOG("Creating CPS2VM\n");
	m_virtualMachine = new CPS2VM();
	m_virtualMachine->Initialize();
	m_virtualMachine->CreatePadHandler(CPH_Generic::GetFactoryFunction());
	m_virtualMachine->CreateGSHandler(CGSH_GXM::GetFactoryFunction());

	auto connection = m_virtualMachine->m_ee->m_os->OnRequestExit.Connect(
		[&executionOver]() {
			executionOver = true;
		});

	LOG("Loading: '%s'\n", file);
	try {
		if (IsBootableExecutablePath(filePath)) {
			m_virtualMachine->m_ee->m_os->BootFromFile(file);
		} else if (IsBootableDiscImagePath(filePath)) {
			CAppConfig::GetInstance().SetPreferencePath(PREF_PS2_CDROM0_PATH, file);
			CAppConfig::GetInstance().Save();
			m_virtualMachine->m_ee->m_os->BootFromCDROM();
		}
	} catch (const std::runtime_error& e) {
		LOG("Exception: %s\n", e.what());
		goto done;
	} catch (...) {
		LOG("Unknown exception\n");
		goto done;
	}

	m_virtualMachine->Resume();

	while (!executionOver) {
		sceCtrlReadBufferPositive(0, &pad, 1);

		if ((pad.buttons & EXIT_MASK) == EXIT_MASK)
			executionOver = true;

		auto padHandler = static_cast<CPH_Generic *>(m_virtualMachine->GetPadHandler());

		padHandler->SetAxisState(PS2::CControllerInfo::ANALOG_LEFT_X, (pad.lx / 255.f) * 2.f - 1.f);
		padHandler->SetAxisState(PS2::CControllerInfo::ANALOG_LEFT_Y, (pad.ly / 255.f) * 2.f - 1.f);
		padHandler->SetAxisState(PS2::CControllerInfo::ANALOG_RIGHT_X, (pad.rx / 255.f) * 2.f - 1.f);
		padHandler->SetAxisState(PS2::CControllerInfo::ANALOG_RIGHT_Y, (pad.ry / 255.f) * 2.f - 1.f);
		padHandler->SetButtonState(PS2::CControllerInfo::DPAD_UP, pad.buttons & SCE_CTRL_UP);
		padHandler->SetButtonState(PS2::CControllerInfo::DPAD_DOWN, pad.buttons & SCE_CTRL_DOWN);
		padHandler->SetButtonState(PS2::CControllerInfo::DPAD_LEFT, pad.buttons & SCE_CTRL_LEFT);
		padHandler->SetButtonState(PS2::CControllerInfo::DPAD_RIGHT, pad.buttons & SCE_CTRL_RIGHT);
		padHandler->SetButtonState(PS2::CControllerInfo::SELECT, pad.buttons & SCE_CTRL_SELECT);
		padHandler->SetButtonState(PS2::CControllerInfo::START, pad.buttons & SCE_CTRL_START);
		padHandler->SetButtonState(PS2::CControllerInfo::SQUARE, pad.buttons & SCE_CTRL_SQUARE);
		padHandler->SetButtonState(PS2::CControllerInfo::TRIANGLE, pad.buttons & SCE_CTRL_TRIANGLE);
		padHandler->SetButtonState(PS2::CControllerInfo::CIRCLE, pad.buttons & SCE_CTRL_CIRCLE);
		padHandler->SetButtonState(PS2::CControllerInfo::CROSS, pad.buttons & SCE_CTRL_CROSS);
		padHandler->SetButtonState(PS2::CControllerInfo::L1, pad.buttons & SCE_CTRL_LTRIGGER);
		padHandler->SetButtonState(PS2::CControllerInfo::R1, pad.buttons & SCE_CTRL_RTRIGGER);
	}

done:
	if (m_virtualMachine) {
		m_virtualMachine->Pause();
		m_virtualMachine->DestroyPadHandler();
		m_virtualMachine->DestroyGSHandler();
		m_virtualMachine->DestroySoundHandler();
		m_virtualMachine->Destroy();
		delete m_virtualMachine;
		m_virtualMachine = nullptr;
	}
}

int main(int argc, char *argv[])
{
	char app_params[1024];
	char picked[1024];
	const char *file = DEFAULT_FILE;
	bool executionOver = false;

	debug_screen_init();
	sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);

	LOG("Play!- Vita port (" PLAY_VERSION ")\n");

	if (!sceAppMgrGetAppParam(app_params)) {
		LOG("App params: %s\n", app_params);
		char *param = strstr(app_params, "&param=");
		if (param) {
			file = param + strlen("&param=");
		} else {
			static const char *supported_ext[] = {
				"elf", "iso", "isz", "cso", "bin", NULL
			};
			char current_dir[1024];
			strcpy(current_dir, "ux0:/Play");

			int ret = file_choose(current_dir, picked, "Choose a PS2 disc/ELF:", supported_ext);
			file = picked;
		}
	}

	vita_rwx_block_create();
	LOG("RWX block UID: 0x%X, size: 0x%X, addr: %p\n",
		rwx_block_uid, RWX_BLOCK_SIZE, rwx_block_addr);

	//sceKernelOpenVMDomain();

	start_emu(file, executionOver);

	//sceKernelCloseVMDomain();

	vita_rwx_block_free();

	if (!executionOver) {
		LOG("\nPress X to exit\n");
		wait_press();
	}

	LOG("\nExiting...\n");

	debug_screen_finish();

	return 0;
}
