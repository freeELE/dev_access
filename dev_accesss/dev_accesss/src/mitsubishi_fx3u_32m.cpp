#include "mitsubishi_fx3u_32m.h"

static void ThdFn(void *args)
{
	Mitsubishi_FX3U_32M *This = static_cast<Mitsubishi_FX3U_32M *>(args);
	This->DoStart();
}

Mitsubishi_FX3U_32M::Mitsubishi_FX3U_32M(stPLCConf * conf)
	: m_bStop(false)
	, m_fn(nullptr)
	, m_pUser(nullptr)
{
	WLogInfo("%s make", __FUNCTION__);
	port = conf->uPort;
	host = conf->szIpAddr;
	id = conf->id;
	interval = conf->interval * 1000;
	mb.reset(new Modbus(host, port));
}

Mitsubishi_FX3U_32M::~Mitsubishi_FX3U_32M()
{
	WLogInfo("%s free", __FUNCTION__);
}

int Mitsubishi_FX3U_32M::Start()
{
	DEBUG_WHERE;
	m_th = thread(ThdFn, this);
	return 0;
}

int Mitsubishi_FX3U_32M::Stop()
{
	m_bStop = true;
	if (m_th.joinable())
	{
		m_th.join();
	}
	mb->ModbusClose();
	return 0;
}


uint16_t Mitsubishi_FX3U_32M::ModbusStart(int address)
{
	uint16_t read_holding_regs = 0;
	try
	{
		mb->ModbusReadHoldingRegisters(address, 1, &read_holding_regs);
	}
	catch (const std::exception& e)
	{
		throw e;
	}
	
	return read_holding_regs;
}



bool Mitsubishi_FX3U_32M::ModbusInit(int id)
{
	mb->ModbusSetSlaveId(id);  
	if (mb->ModbusConnect())
	{
		return true;
	}
	else
	{
		return false;
	}
}

void Mitsubishi_FX3U_32M::SetEventCallback(EventMsgFn fn, void * pUser)
{
	m_fn = fn;
	m_pUser = pUser;
}


void Mitsubishi_FX3U_32M::DoStart()
{
	int i = 0;
	int IntSet[8] = { 2020,3000,3004,3002,2022,3100,3104,3102};
	uint16_t OldValue[8] = {0};
	uint16_t TempValue = 0;
	bool Update = false;
	int unixTime;
	time_t tick;
	time_t now;
	struct tm tm;

	struct Mitsubishi_FX3U_32M_Data data;
	while (true)
	{
		while (!ModbusInit(id)) //测试
		{
			WLogError("%s:%d ModbusInit failure .....", __FUNCTION__, __LINE__);
			Sleep(5000);
		}


		while (!m_bStop)
		{
			for (i = 0; i < sizeof(IntSet) / sizeof(IntSet[0]); i++)
			{
				try
				{
					TempValue = ModbusStart(IntSet[i]);
				}
				catch (const std::exception& e)
				{
					m_bStop = true;
					break;
				}

				if (OldValue[i] != TempValue)
				{
					OldValue[i] = TempValue;
					Update = true;
				}
			}
			if (Update)
			{
				Update = false;
				if (m_fn)
				{
					unixTime = (int)time(&now);
					tick = (time_t)unixTime;
					tm = *localtime(&tick);
					strftime(data.Timestamp, sizeof(data.Timestamp), "%Y-%m-%d %H:%M:%S", &tm);
					sprintf(data.DevInfo, host, "@", port);
					data.StationStatus_1 = OldValue[0];
					data.StationOkAmount_1 = OldValue[1];
					data.StationTotalAmount_1 = OldValue[2];
					data.StationNgAmount_1 = OldValue[3];
					data.StationStatus_2 = OldValue[4];
					data.StationOkAmount_2 = OldValue[5];
					data.StationTotalAmount_2 = OldValue[6];
					data.StationNgAmount_2 = OldValue[7];
					cout << "1号工站状态 = " << OldValue[0] << endl;
					cout << "1号工站OK数量 = " << OldValue[1] << endl;
					cout << "1号工站测试总数量 = " << OldValue[2] << endl;
					cout << "1号工站NG数量 = " << OldValue[3] << endl;
					cout << "2号工站状态 = " << OldValue[4] << endl;
					cout << "2号工站OK数量 = " << OldValue[5] << endl;
					cout << "2号工站测试总数量 = " << OldValue[6] << endl;
					cout << "2号工站NG数量 = " << OldValue[7] << endl;
					cout << data.Timestamp << endl;
					m_fn(eEVENT_MITSUBISHI_FX3U_32M, (void *)&data, m_pUser);
				}
			}
			Sleep(interval);
		}
		m_bStop = false;
		memset(OldValue, 0, sizeof(OldValue));
	}
}