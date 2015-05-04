#ifndef __BONDRIVER_PROXYEX_H__
#define __BONDRIVER_PROXYEX_H__
#include <winsock2.h>
#include <ws2tcpip.h>
#include <tchar.h>
#include <process.h>
#include <list>
#include <queue>
#include <map>
#include "Common.h"
#include "IBonDriver3.h"

#if _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

#define WAIT_TIME	10	// GetTsStream()の後で、dwRemainが0だった場合に待つ時間(ms)

////////////////////////////////////////////////////////////////////////////////

static char g_Host[256];
static char g_Port[8];
static size_t g_PacketFifoSize;
static DWORD g_TsPacketBufSize;
static DWORD g_OpenTunerRetDelay;
static BOOL g_SandBoxedRelease;
static BOOL g_DisableUnloadBonDriver;

#include "BdpPacket.h"

#define MAX_DRIVERS	64		// ドライバのグループ数とグループ内の数の両方
static char **g_ppDriver[MAX_DRIVERS];
struct stDriver {
	char *strBonDriver;
	HMODULE hModule;
	BOOL bUsed;
	FILETIME ftLoad;
};
static std::map<char *, std::vector<stDriver> > DriversMap;

////////////////////////////////////////////////////////////////////////////////

struct stTsReaderArg {
	IBonDriver *pIBon;
	volatile BOOL StopTsRead;
	volatile BOOL ChannelChanged;
	DWORD pos;
	std::list<cProxyServerEx *> TsReceiversList;
	cCriticalSection TsLock;
	stTsReaderArg()
	{
		StopTsRead = FALSE;
		ChannelChanged = TRUE;
		pos = 0;
	}
};

class cProxyServerEx {
	IBonDriver *m_pIBon;
	IBonDriver2 *m_pIBon2;
	IBonDriver3 *m_pIBon3;
	HMODULE m_hModule;
	SOCKET m_s;
	cEvent m_Error;
	BOOL m_bTunerOpen;
	HANDLE m_hTsRead;
	BOOL m_bChannelLock;
	stTsReaderArg *m_pTsReaderArg;
#if !BUILD_AS_SERVICE && _DEBUG
public:
#endif
	DWORD m_dwSpace;
	DWORD m_dwChannel;
	char *m_pDriversMapKey;
	int m_iDriverNo;
	int m_iDriverUseOrder;
	cPacketFifo m_fifoSend;
	cPacketFifo m_fifoRecv;

#if !BUILD_AS_SERVICE && _DEBUG
private:
#endif
	DWORD Process();
	int ReceiverHelper(char *pDst, DWORD left);
	static DWORD WINAPI Receiver(LPVOID pv);
	void makePacket(enumCommand eCmd, BOOL b);
	void makePacket(enumCommand eCmd, DWORD dw);
	void makePacket(enumCommand eCmd, LPCTSTR str);
	void makePacket(enumCommand eCmd, BYTE *pSrc, DWORD dwSize, float fSignalLevel);
	static DWORD WINAPI Sender(LPVOID pv);
	static DWORD WINAPI TsReader(LPVOID pv);
	void StopTsReceive();

	BOOL SelectBonDriver(LPCSTR p);
	IBonDriver *CreateBonDriver();

	// IBonDriver
	const BOOL OpenTuner(void);
	void CloseTuner(void);
	void PurgeTsStream(void);
	void Release(void);

	// IBonDriver2
	LPCTSTR EnumTuningSpace(const DWORD dwSpace);
	LPCTSTR EnumChannelName(const DWORD dwSpace, const DWORD dwChannel);
	const BOOL SetChannel(const DWORD dwSpace, const DWORD dwChannel);

	// IBonDriver3
	const DWORD GetTotalDeviceNum(void);
	const DWORD GetActiveDeviceNum(void);
	const BOOL SetLnbPower(const BOOL bEnable);

public:
	cProxyServerEx();
	~cProxyServerEx();
	void setSocket(SOCKET s){ m_s = s; }
	static DWORD WINAPI Reception(LPVOID pv);
};

static std::list<cProxyServerEx *> g_InstanceList;
static cCriticalSection g_Lock;
static cEvent g_TerminateRequest(TRUE, FALSE);
#if BUILD_AS_SERVICE
// サービスのメイン
void WINAPI ServiceMain(DWORD argc, LPTSTR *argv);
// サービスコントロールハンドラ
DWORD WINAPI HandlerEx(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext);

class CWinService
{
public:
	CWinService();
	virtual ~CWinService();

	// インストールとアンインストール
	BOOL Install();
	BOOL Remove();

	// 起動・停止・再起動
	BOOL Start();
	BOOL Stop();
	BOOL Restart();

	// 実行
	BOOL Run(LPSERVICE_MAIN_FUNCTIONW lpServiceProc);

	DWORD WINAPI ServiceCtrlHandler(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext);

	BOOL RegisterService(LPHANDLER_FUNCTION_EX lpHandlerProc);
	void ServiceRunning();
	void ServiceStopped();

private:
	SERVICE_STATUS serviceStatus;
	SERVICE_STATUS_HANDLE serviceStatusHandle;

	//サービス名称保持
	TCHAR serviceName[BUFSIZ];
	TCHAR serviceExePath[BUFSIZ];

	//終了イベント
	HANDLE  hServerStopEvent;
};

static CWinService *lpCWinService = NULL;
#endif

#endif	// __BONDRIVER_PROXYEX_H__
