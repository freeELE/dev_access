//#include "stdafx.h"
#include "wLog.h"
#include "../Locker/internallock.h"

#ifndef WIN32
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/statvfs.h>
#include <dirent.h>
#include <unistd.h>
#include <fnmatch.h>
#else
#include <tchar.h>
#include <Windows.h>
#include <Psapi.h>
#pragma comment(lib,"Psapi.lib")
#endif

#include <time.h>
#include <queue>
#include <string>
#include <sstream>
#include <iomanip>
using namespace std;

namespace __W_LOG_PLAY
{
	static  time_t g_last_printfsystem_time = 0;

	struct LogData
	{
	private:
		bool m_bInit;

		int m_iMinFiles;
		int m_iMaxFiles;
		int m_iReserveByte;
		int m_file_bytes_limit;
		char m_szDir[260];
		char m_szFileName[128];

		int m_iLogSize;
		int m_iFileCount;
		std::queue<std::string> m_FileList;

		FILE *m_FileFD;
		INT64  m_lCleanTick;

	public:
		CResGuard m_lock;

		typedef char LOGTITLE[MAX_EVENT_TITLE];
		LOGTITLE m_titles[MAX_EVENT_TYPE];

		char *m_pBuffer;
		int  m_iBufferLen;

		FILE *m_Nullfd;

	public:
		LogData()
		{
			m_bInit = true;
			m_lCleanTick = 0;
			m_iLogSize = 0;
			m_FileFD = NULL;
			m_Nullfd = NULL;

			m_iFileCount = 0;
			m_iMinFiles = DEFAULT_MIN_LOGFILE_SIZE;
			m_iMaxFiles = DEFAULT_MAX_LOGFILE_SIZE;
			m_iReserveByte = DEFAULT_DISC_RESERVE_MBYTES;
			m_file_bytes_limit = DEFAULT_FILE_SIZE;
			memset(m_szDir, 0, sizeof(m_szDir));
			sprintf(m_szDir, "./Log");
			strcpy(m_szFileName, "wlog");

			sprintf(m_titles[LOGTYPE_INFO], "Info");
			sprintf(m_titles[LOGTYPE_WARN], "Warn");
			sprintf(m_titles[LOGTYPE_ERROR], "Error");
			sprintf(m_titles[LOGTYPE_DEBUG], "Debug");

			m_iBufferLen = 1024*16;
			m_pBuffer = (char *)malloc(m_iBufferLen);
		}

		~LogData()
		{
			if(m_pBuffer)
			{
				free(m_pBuffer);
				m_pBuffer = NULL;
			}

			if (m_Nullfd != NULL)
			{
				fclose(m_Nullfd);
				m_Nullfd = NULL;
			}

			if (m_FileFD != NULL)
			{
				fclose(m_FileFD);
				m_FileFD = NULL;
			}
		}

		static LogData &Instance();

		int Init(const char *pDir, const char *pFileName, int iMinFiles, int iMaxFiles, int iReserveMBytes, bool bLog);
		int OpenDir(const char *pDir);
		int TestDir(const char *pDir);

		void SerarchLogsByDir();
		bool SwitchNewFile();
		bool WriteLog(const char *pBuffer, int iLen);

		bool MatchingString(const char *string, const char *pattern);
		bool IsExistDir(const char *lpsz);
		bool DeleteFile_(const char *lpfile);
		void CleanFile_();
		INT64 GetFileSize_(const char *lpszFile);
		INT64 GetDiskFreeSpace_(const char *lpszRootPath);
		INT64 GetBootTickCount();
	};

	LogData &LogData::Instance()
	{
		static LogData sData;
		return sData;
	}

	void LogData::SerarchLogsByDir()
	{
#ifdef WIN32
		class CFileSearch
		{
		private:
			WIN32_FIND_DATA find_data_;
			HANDLE handle_;
			char pathname_[260];
			char *pathEnd_;

		public:
			CFileSearch() : handle_(INVALID_HANDLE_VALUE), pathEnd_(NULL) {}
			~CFileSearch()
			{
				Close();
			}
			BOOL IsOK()
			{
				return handle_ != INVALID_HANDLE_VALUE;
			}

			void Close()
			{
				if(handle_ != INVALID_HANDLE_VALUE)
				{
					::FindClose(handle_);
					handle_ = INVALID_HANDLE_VALUE;
				}
			}

			BOOL FindFile(LPCTSTR pstrName)
			{
				char szFileName[260];
				sprintf(szFileName, "%s/*", pstrName);
				handle_ = ::FindFirstFile(szFileName, &find_data_);

				if(handle_ != INVALID_HANDLE_VALUE)
				{
					if(_tfullpath(pathname_, szFileName, _MAX_PATH))
					{
						char *pstr = _tcsrchr(pathname_, '\\');

						if(pstr == NULL)
							pstr = _tcsrchr(pathname_, '/');

						if(pstr != NULL)
						{
							pathEnd_ = pstr + 1;
							_tcscpy(pathEnd_, find_data_.cFileName);
							return TRUE;
						}
					}

					Close();
				}

				return FALSE;
			}

			DWORD GetAttributes() const
			{
				return find_data_.dwFileAttributes;
			}

			BOOL MatchesMask(DWORD dwMask) const
			{
				return !!(find_data_.dwFileAttributes & dwMask);
			}

			BOOL IsDirectory() const
			{
				return MatchesMask(FILE_ATTRIBUTE_DIRECTORY);
			}

			BOOL IsDots() const
			{
				if(IsDirectory())
				{
					if(find_data_.cFileName[0] == '.')
					{
						if(find_data_.cFileName[1] == '\0' ||
						        (find_data_.cFileName[1] == '.' && find_data_.cFileName[2] == '\0'))
						{
							return TRUE;
						}
					}
				}

				return FALSE;
			}

			LPCTSTR GetFileName() const
			{
				return find_data_.cFileName;
			}

			LPCTSTR GetFilePath() const
			{
				return pathname_;
			}

			BOOL FindNextFile()
			{
				BOOL ret = ::FindNextFile(handle_, &find_data_);

				if(ret)
					_tcscpy(pathEnd_, find_data_.cFileName);

				return ret;
			}
		};

		CFileSearch finder;
		char szPathName[260];
		char szSearchFileName[260];//SamsungSDK*.log
		sprintf(szSearchFileName, "%s*.log", m_szFileName);
		bool dir_have_slash = false;

		if(m_szDir[strlen(m_szDir) - 1] == '/')
		{
			dir_have_slash = true;
		}

		for(BOOL bWorking = finder.FindFile(m_szDir);
		        bWorking;
		        bWorking = finder.FindNextFile()
		   )
		{
			if(finder.IsDots())
				continue;

			LPCTSTR filename = finder.GetFileName();
			sprintf(szPathName, "%s%s%s", m_szDir, dir_have_slash ? "" : "/", filename);
			BOOL isDir = finder.IsDirectory();

			if(!isDir)
			{
				bool bMatch = MatchingString(filename, szSearchFileName);

				if(bMatch)
				{
					CResGuard::Defend AutoLocker(m_lock);
					m_iFileCount++;
					m_FileList.push(szPathName);
				}
			}
		}

#else
		struct TDirent
		{
			struct dirent **namelist;
			int count;
			TDirent() : namelist(NULL), count(0) {}
			~TDirent()
			{
				if(namelist)
				{
					for(int i = 0; i < count; i++)
						::free(namelist[i]);

					::free(namelist);
				}
			}

			static int filter(const struct dirent *nl)
			{
				if(0 == strcmp(nl->d_name, ".") || 0 == strcmp(nl->d_name, ".."))
					return 0;
				else
					return 1;
			}
		};

		TDirent rent;
		rent.count = scandir(m_szDir, &rent.namelist, TDirent::filter, alphasort);

		if(rent.count < 0)
		{
			return;
		}

		bool dir_have_slash = false;

		if(m_szDir[strlen(m_szDir) - 1] == '/')
		{
			dir_have_slash = true;
		}

		char szPathName[260];
		char szSearchFileName[260];//SamsungSDK*.log
		sprintf(szSearchFileName, "%s*.log", m_szFileName);

		for(int i = 0; i < rent.count; i++)
		{
			sprintf(szPathName, "%s%s%s", m_szDir, dir_have_slash ? "" : "/", rent.namelist[i]->d_name);
			bool isDir = IsExistDir(szPathName);

			if(!isDir)
			{
				bool bMatch = MatchingString(rent.namelist[i]->d_name, szSearchFileName);

				if(bMatch)
				{
					CResGuard::Defend AutoLocker(m_lock);
					m_iFileCount++;
					m_FileList.push(szPathName);
				}
			}
		}

#endif
	}

	int LogData::Init(const char *pDir, const char *pFileName, int iMinFiles, int iMaxFiles, int iReserveMBytes, bool bLog)
	{
		if (!bLog)
		{
			m_bInit = false;
			return 0;
		}

		if(pDir != NULL)
		{
			sprintf(m_szDir, "%s", pDir);
		}

		m_iMinFiles = iMinFiles;
		m_iMaxFiles = iMaxFiles;
		m_iReserveByte = iReserveMBytes * 1024 * 1024;

		if(pFileName)
		{
			strcpy(m_szFileName, pFileName);
		}

		int iRet = TestDir(pDir);

		if(iRet == 0)
		{
			SerarchLogsByDir();
		}
		else if(iRet == -1)
		{
			m_bInit = false;
			return -1;
		}

		return 0;
	}

	int LogData::TestDir(const char *pDir)
	{
		bool bRet = IsExistDir(pDir);
		if(bRet)
		{
			return 0;
		}

#ifdef WIN32
		BOOL bCreate = CreateDirectory(pDir, NULL);
		if(bCreate)
		{
			return 1;
		}
#else
		int iMk = mkdir(pDir, 0777);
		if(iMk == 0)
		{
			return 1;
		}
#endif

		return -1;
	}

	bool LogData::DeleteFile_(const char *lpfile)
	{
		if ( 0 == unlink(lpfile) )
			return true;
		return false;
	}

	bool LogData::SwitchNewFile()
	{
		if(m_iLogSize > m_file_bytes_limit)
		{
			if(m_FileFD != NULL)
			{
				fclose(m_FileFD);
				m_FileFD = NULL;
			}
		}

		if(m_FileFD == NULL)
		{
			time_t nowtime;
			struct tm *t;
			::time(&nowtime);
			t = localtime(&nowtime);

			char szFileName[260];
			sprintf(szFileName, "%s/%s%04d%02d%02d%02d%02d%02d.log",
			        m_szDir, m_szFileName,
			        t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
			        t->tm_hour, t->tm_min, t->tm_sec);

			m_FileFD = fopen(szFileName, "wb");

			if(m_FileFD != NULL)
			{
				m_iFileCount++;
				m_FileList.push(szFileName);

				m_iLogSize = 0;
			}
		}

		return (m_FileFD != NULL);
	}

	void LogData::CleanFile_()
	{
		INT64 ltNow = GetBootTickCount();
		if(ltNow - m_lCleanTick > CLEAN_TIME_INTERVAL)
		{
			while(m_iFileCount >= m_iMaxFiles && m_iFileCount > m_iMinFiles)
			{
				std::string &strFileName = m_FileList.front();
				DeleteFile_(strFileName.c_str());
				m_FileList.pop();
				--m_iFileCount;
			}

			INT64 totalFreeBytes = GetDiskFreeSpace_(m_szDir);
			INT64 lNeedClean = (INT64)m_iReserveByte - totalFreeBytes;
			if(lNeedClean > 0)
			{
				while(m_FileList.size() > 0 && m_iFileCount > m_iMinFiles)
				{
					std::string &strFileName = m_FileList.front();
					INT64 iFileSize = GetFileSize_(strFileName.c_str());
					DeleteFile_(strFileName.c_str());

					m_FileList.pop();
					--m_iFileCount;
					lNeedClean -= iFileSize;
					if(lNeedClean <= 0)
					{
						break;
					}
				}
			}

			m_lCleanTick = ltNow;
		}
	}

	bool LogData::WriteLog(const char *pBuffer, int iLen)
	{
		if(!m_bInit)
		{
			return false;
		}

		CleanFile_();

		if(SwitchNewFile())
		{
			//ע:��ʹ�ļ���ɾ��,fwrite��fflush��Ȼ������ȷ.����,����Ҫ���ж���.
			//if (fwrite(pBuffer, iLen, 1, m_FileFD) > 0)
			{
				fwrite(pBuffer, iLen, 1, m_FileFD);
				fflush(m_FileFD);
				m_iLogSize += iLen;
				return true;
			}

			m_iLogSize = m_file_bytes_limit + 1;
		}

		return false;
	}
	//////////////////////////////////////////////////////////////////////////

	bool LogData::IsExistDir(const char *lpsz)
	{
		if(lpsz == NULL)
			return false;

#ifdef WIN32
		DWORD dwRet = GetFileAttributes(lpsz);
		return ((dwRet != INVALID_FILE_ATTRIBUTES) && (FILE_ATTRIBUTE_DIRECTORY == (FILE_ATTRIBUTE_DIRECTORY & dwRet)));
#else
		struct stat fileinfo;
		int ret = ::stat(lpsz, &fileinfo);
		return (ret == 0 && S_IFDIR == (fileinfo.st_mode & S_IFDIR));
#endif
	}

	INT64 LogData::GetFileSize_(const char *lpszFile)
	{
		if(lpszFile == NULL)
			return 0;

#ifdef WIN32
		HANDLE hFile = CreateFile(lpszFile, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
		                          OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);

		if(INVALID_HANDLE_VALUE == hFile)
			return 0;

		DWORD dwLSize, dwHSize;
		dwLSize = ::GetFileSize(hFile, &dwHSize);

		if(INVALID_FILE_SIZE == dwLSize && NO_ERROR != GetLastError())
		{
			CloseHandle(hFile);
			return 0;
		}

		CloseHandle(hFile);
		return (INT64(dwHSize) << 32) + INT64(dwLSize);
#else
		struct stat fileinfo;
		int ret = ::stat(lpszFile, &fileinfo);

		if(ret == 0)
			return fileinfo.st_size;
		else
			return 0;

#endif
	}

	INT64 LogData::GetDiskFreeSpace_(const char *lpszRootPath)
	{
#ifdef WIN32
		ULARGE_INTEGER free_use = {0};
		ULARGE_INTEGER totals = {0};
		ULARGE_INTEGER frees = {0};

		if(::GetDiskFreeSpaceEx(lpszRootPath,
		                        (PULARGE_INTEGER)&free_use,
		                        (PULARGE_INTEGER)&totals,
		                        (PULARGE_INTEGER)&frees))
		{
			return free_use.QuadPart;
		}

		DWORD a = 0;
		DWORD b = 0;
		DWORD c = 0;
		DWORD d = 0;

		if(::GetDiskFreeSpace(lpszRootPath, &a, &b, &c, &d))
		{
			return a * b * c;
		}

		return 0;
#else
		struct statvfs info;
		int status = ::statvfs(lpszRootPath, &info);

		if(status >= 0)
		{
			return (INT64)info.f_frsize * (INT64)info.f_bavail;
		}

		return 0;
#endif
	}

	bool LogData::MatchingString(const char *string, const char *pattern)
	{
#ifdef WIN32

		bool bMatch = true;
		char *lpstr = NULL;
		char szTemp[260] = {0};
		char *pPatternTmp = (char *)szTemp;
		memcpy(szTemp, pattern, strlen(pattern));

		do
		{
			lpstr = strstr(pPatternTmp, "*");

			if(lpstr == NULL)
			{
				if(strlen(pPatternTmp) > 0)
				{
					if(strstr(string, pPatternTmp) == NULL)
					{
						bMatch = false;
					}
				}

				break;
			}

			int offset = lpstr - pPatternTmp;
			pPatternTmp[offset] = '\0';

			if(strstr(string, pPatternTmp) == NULL)
			{
				bMatch = false;
				break;
			}

			pPatternTmp += (offset + 1);
		}
		while(1);

		return bMatch;
#else
		int ret = fnmatch(pattern, string, 0);
		return (ret == 0);
#endif
	}

	INT64 LogData::GetBootTickCount()
	{
#if defined(WIN32)
		static DWORD lastTickCount = 0;
		static DWORD cycles = 0;

		DWORD curTickCount = GetTickCount();
		INT64 ret = curTickCount;

		// GetTickCount ��λ��Ĵ���
		if(lastTickCount > curTickCount)
		{
			++cycles;
		}

		ret |= ((INT64)(cycles) << 32);
		lastTickCount = curTickCount;

		return ret;

#else
		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);

		return (ts.tv_sec * 1000 + ts.tv_nsec / (1000 * 1000));
#endif
	}


	void Init(const char *pDir, const char *pFileName, int iMinFiles, int iMaxFiles, int iReserveMBytes, bool bLog)
	{
		LogData &log = LogData::Instance();
		log.Init(pDir, pFileName, iMinFiles, iMaxFiles, iReserveMBytes, bLog);
	}

	void WriteLog(int iType, const char *pFun, int line, const char *fmt, ...)
	{
		va_list arglst;
		va_start(arglst, fmt);
		vWriteLog(iType,pFun,line,fmt,arglst);
		va_end(arglst);
	}

	CResGuard &GetLock()
	{
		LogData &log = LogData::Instance();
		return log.m_lock;
	}

	void vWriteLog(int iType,const char *pFun, int line, const char *fmt, void *_va_)
	{
#if !defined(__debug__)
		if(iType == LOGTYPE_DEBUG)
			return;
#endif

		LogData &log = LogData::Instance();
		CResGuard::Defend AutoLocker(log.m_lock);

		time_t nowtime;
		struct tm *t;
		::time(&nowtime);
		t = localtime(&nowtime);
		if (t == NULL)
		{
			return;
		}

		int iBufferLen = log.m_iBufferLen;
		char *pBuffer = log.m_pBuffer;
		int iTotalLen = 0;
		int iWhile = 2;

		do
		{
			int iLen = plat_snprintf(pBuffer, iBufferLen, "%04d-%02d-%02d %02d:%02d:%02d t%d|%s:"
				, t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
				t->tm_hour, t->tm_min, t->tm_sec
				, GetCurrentThreadId()
				, log.m_titles[iType]);

			int iLimit = iBufferLen-iLen-2;
			va_list arglst = (va_list)_va_;
			//va_start(arglst, fmt);
			int iLen2 = vsnprintf(pBuffer + iLen, iLimit, fmt, arglst);
			//va_end(arglst);

			if (iLen2 < 0 || iLen2 > iLimit)
			{ //log.m_pBuffer�ĳ���С�ڸ�ʽ������Ĵ�С
				
				iLen2 = iLimit;

				//�˴�Ϊ����������棬��ʱ����Ҫ��ֱ�ӽضϰɡ�
				/*int iFormatLen = GetFormatLen(fmt, arglst);
				va_end(arglst);

				iBufferLen = LOG_ADJUST_32BITS(iFormatLen) + 64;

				free(pBuffer);
				pBuffer = (char *)malloc(iBufferLen);
				log.m_pBuffer = pBuffer;
				log.m_iBufferLen = iBufferLen;
				continue;*/
			}
			
			
			iTotalLen = iLen + iLen2;
			if(pBuffer[iTotalLen - 1] != '\n')
			{
				pBuffer[iTotalLen] = '\n';
				iTotalLen++;
			}

			pBuffer[iTotalLen] = 0;
			log.WriteLog(pBuffer, iTotalLen);

			printf("%s", pBuffer);
#if defined(WIN32) && defined(_DEBUG)
			OutputDebugStringA(pBuffer);
#endif
			break;
		}
		while(iWhile-- > 0);
	
		int iDecTimes = (int)(nowtime - g_last_printfsystem_time);
		if (iDecTimes > 60)
		{
			g_last_printfsystem_time = nowtime;
			__W_LOG_PLAY::PrintfSystemInfo();//��ӡ�ڴ�ռ����Ϣ
		}

	}


	int GetFormatLen(const char* fmt, va_list argList)
	{
#if defined(WIN32) && (_MSC_VER >= 1500) // >= VC2008
		int len = _vscprintf(fmt, argList);
#else
		static FILE* fp = NULL;
		if(NULL == fp)
		{
			LogData &log = LogData::Instance();
			#if defined(WIN32)
				log.m_Nullfd = fopen("nul", "wb");
			#else // __linux__
				log.m_Nullfd = fopen("/dev/null", "w");
			#endif
			fp = log.m_Nullfd;
		}
		int len = vfprintf(fp, fmt, argList) 
		#if defined(__linux__)
			+ 64 // ���64�ֽڣ��ݴ���linux�·�����Щ���vfprintf()���صĳ��Ȼ��sprintf()���صĳ����٣�
		#endif 
			;
#endif
		return len;
	}

	//��ʮ����������ת��Ϊ�ַ���.
	//����: �ַ����ĳ���.pOutStr����ת������ַ���
	//ÿһ��ʮ�������ַ�ռ3���ַ�,�ⲿ�豣֤ pOutStr �Ĵ�С������ iLen*3+2;
	int HexArrayToString(char *pOutStr, void *hex_array, int iLen)
	{
		if(!pOutStr || !hex_array || !iLen)
		{
			return 0;
		}

		int iDataLen = 0;
		unsigned char *pArray = (unsigned char *)hex_array;

		for(int i = 0; i < iLen; i++)
		{
			iDataLen += sprintf(pOutStr + iDataLen, "%02x ", pArray[i]);
		}

		return iDataLen;
	}

	int HexArrayToString(std::string &outstr, void *hex_array, int iLen)
	{
		if(!hex_array || !iLen)
		{
			outstr = "";
			return 0;
		}

		std::stringstream ss;
		ss << std::hex << std::setw(2) << std::setfill('0');
		unsigned char *pArray = (unsigned char *)hex_array;

		for(int i = 0; i < iLen; i++)
		{
			ss << (int)(pArray[i]) << " ";
		}

		outstr = ss.str();
		return outstr.size();
	}

	int PrintfSystemInfo()
	{
#ifdef WIN32
		MEMORYSTATUSEX stat;
		stat.dwLength = sizeof(stat);
		GlobalMemoryStatusEx(&stat);

		HANDLE h = GetCurrentProcess();
		PROCESS_MEMORY_COUNTERS pmc;
		GetProcessMemoryInfo(h,&pmc,sizeof(pmc));
		
		typedef unsigned long ul;

		ul Mb = 1024 * 1024;
		ul iSysMemPre = (ul)(stat.dwMemoryLoad);//�ڴ�ռ�ðٷֱ�
		ul iSysTotal = (ul)(stat.ullTotalPhys / Mb);
		ul iSysEmpty = (ul)(stat.ullAvailPhys / Mb);
		

		ul iMaxSize = (ul)(pmc.PeakWorkingSetSize / Mb);
		ul iCurrSize = (ul)(pmc.WorkingSetSize / Mb);

		ul iVCur = (ul)(pmc.PagefileUsage / Mb);
		ul iVMax = (ul)(pmc.PeakPagefileUsage / Mb);

		WriteLog(0,NULL,0,"[System]mem:total:%uM use:%u%% free:%uM pro:Phy:%uM/%uM,Vir:%uM/%uM"
			,iSysTotal,iSysMemPre,iSysEmpty
			,iCurrSize,iMaxSize
			,iVCur,iVMax
			);
#endif
		return 0;
	}
};


