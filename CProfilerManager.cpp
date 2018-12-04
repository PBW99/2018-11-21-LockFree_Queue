#include "SharedPCH.h"
CProfilerManager * g_cProfileM;

uint64_t CProfilerManager::sfreq;
char     CProfilerManager::sFileDir[50];
bool     CProfilerManager::sOnOff = false;

void CProfilerManager::Setup(bool onoff, const char * fileDir, int threadNum)
{
	CProfilerManager::sOnOff = onoff;
	strncpy_s(CProfilerManager::sFileDir, fileDir,_countof(CProfilerManager::sFileDir)-1);
	CreateDirectoryA(CProfilerManager::sFileDir, NULL);
	g_cProfileM = CProfilerManager::GetCProfilerManager(threadNum);
}

void CProfilerManager::BeginTag(const char * tagName)
{
	if (!sOnOff)return;
	
	// Get Thread's CProfiles
	void * tlsval = TlsGetValue(mTlsIdx);
	if (tlsval == nullptr)
	{
		// Enroll Thread's CProfiles
		AcquireSRWLockExclusive(&mCprofsSrwlock);
		for (int i = 0; i < mCprofileIncnt; ++i)
		{
			if (!mCprofiles[i].isValid)
			{
				mCprofiles[i].isValid = true;
				mCprofiles[i].tid     = GetCurrentThreadId();
				TlsSetValue(mTlsIdx, &mCprofiles[i]);
				tlsval = &mCprofiles[i];
				mCprofileIncnt++;
				break;
			}
		}
		ReleaseSRWLockExclusive(&mCprofsSrwlock);
	}
	CProfilers * targetCps   = (CProfilers *)tlsval;
	CProfiler *  targetPfArr = targetCps->cprofiles;

	// Find Thread's Function Profile
	CProfiler * target = nullptr;
	for (int    i      = 0; i < targetCps->cprofilesIncnt; ++i)
	{
		if (targetPfArr[i].isValid && strncmp(targetPfArr[i].mTagName, tagName,dfPROFILE_FUNCNAME_MAX)
			==
			0)
		{
			target = &targetPfArr[i];
			break;
		}
	}
	if (target == nullptr)
	{
		// Enroll Thread's Function Profile
		int targeti  = 0;
		for (targeti = 0; targeti < targetCps->cprofilesIncnt; ++targeti)
		{
			if (!targetPfArr[targeti].isValid)
			{
				targetCps->cprofilesIncnt++;
				break;
			}
		}
		if (targeti == dfPROFILE_FUNCNUM_MAX)
		{
			LOG("Engine", dfLOG_LEVEL_ERROR,"Over ProfileNum Errr!!");
		}
		target = &targetPfArr[targeti];
		target->InitCProfiler(tagName);
	}
	target->Begin();
}

bool CProfilerManager::EndTag(const char * tagName)
{
	if (!sOnOff)return false;
	// Get Thread's CProfiles
	void * tlsval = TlsGetValue(mTlsIdx);
	if (tlsval == nullptr)
	{
		LOG("Engine", dfLOG_LEVEL_ERROR, "Tls Error");
	}
	CProfilers * targetCps   = (CProfilers *)tlsval;
	CProfiler *  targetPfArr = targetCps->cprofiles;

	// Find Thread's Function Profile
	CProfiler * target = nullptr;
	for (int    i      = 0; i < targetCps->cprofilesIncnt; ++i)
	{
		if (targetPfArr[i].isValid && strncmp(targetPfArr[i].mTagName, tagName, dfPROFILE_FUNCNAME_MAX)
			==
			0)
		{
			target = &targetPfArr[i];
			break;
		}
	};

	if (target == nullptr)
	{
		return false;
	}


	bool ret;
	ret = target->End();
	return ret;
}

void CProfilerManager::OutSummaryToFile()
{
	if (!sOnOff)return;
	FILE * file;
	int    ret = 1;

	struct tm tm;
	time_t    rawtime;

	time(&rawtime);
	localtime_s(&tm, &rawtime);

	char outFileName[100];
	sprintf_s(outFileName,
	          "%s/Profile_%04d%02d%02d%02d%02d.csv",
	          CProfilerManager::sFileDir,
	          tm.tm_year + 1900,
	          tm.tm_mon + 1,
	          tm.tm_mday,
	          tm.tm_hour,
	          tm.tm_min,
	          tm.tm_sec
	         );

	ret = fopen_s(&file, outFileName, "w+");
	if (ret != 0)
	{
		LOG("Engine", dfLOG_LEVEL_ERROR, "File open err\n");
		return;
	}


	// Generate Sum
	ReducCProfilers   sumProfiles;
	ReducCProfilers * sumCps   = &sumProfiles;
	ReducCProfiler *  sumPfArr = sumCps->cprofiles;
	for (int          outi     = 0; outi < mCprofileIncnt; ++outi)
	{
		CProfilers * thdCps = &mCprofiles[outi];

		if (thdCps->isValid)
		{
			CProfiler * thdPfArr = thdCps->cprofiles;

			for (int i = 0; i < thdCps->cprofilesIncnt; ++i)
			{
				if (thdPfArr[i].isValid)
				{
					CProfiler * thdFuncCp = &thdPfArr[i];

					// Find Thread's Function Profile
					ReducCProfiler * sumFuncCp = nullptr;
					for (int         i         = 0; i < sumCps->cprofilesIncnt;
					     ++i
					)
					{
						if (sumPfArr[i].isValid &&
							strncmp(sumPfArr[i].mTagName, thdFuncCp->mTagName, dfPROFILE_FUNCNAME_MAX)
							==
							0)
						{
							sumFuncCp = &sumPfArr[i];
							break;
						}
					}
					if (sumFuncCp == nullptr)
					{
						// Enroll Thread's Function Profile
						int targeti  = 0;
						for (targeti = 0; targeti < sumCps->cprofilesIncnt;
						     ++targeti)
						{
							if (!sumPfArr[targeti].isValid)
							{
								sumCps->cprofilesIncnt++;
								break;
							}
						}
						if (targeti == dfPROFILE_FUNCNUM_MAX)
						{
							LOG("Engine", dfLOG_LEVEL_ERROR, "Over ProfileNum Errr!!");
						}
						sumFuncCp = &sumPfArr[targeti];
						sumFuncCp->InitCProfiler(thdFuncCp->mTagName);
					}
					// Proc Min Max Avr
					sumFuncCp->ProcMin(thdFuncCp->min[0]);
					sumFuncCp->ProcMax(thdFuncCp->max[0]);
					sumFuncCp->ProcTotal(thdFuncCp->total, thdFuncCp->callNum);
				}
			}
		}
	}

	// Print Sum
	fprintf(file,
	        "ByoungWook Park  Custom Profiler\n");
	fprintf(file,
	        "<<S      U      M>>=====================================================================================================================\n");
	fprintf(file,
	        " Tid  , Name                                             ,           Average  ,           1stMin   ,           1stMax   ,          Call \n");

	for (int i = 0; i < sumCps->cprofilesIncnt; ++i)
	{
		if (sumPfArr[i].isValid)
		{
			ReducCProfiler * target = &sumPfArr[i];
			fprintf(file,
			        "%6d,%-50s,%18lf㎲,%18lf㎲,%18lf㎲,%15llu\n",
			        sumCps->tid,
			        target->mTagName,
			        target->GetAverageInMicro(),
			        target->GetMinInMicro(),
			        target->GetMaxInMicro(),
			        target->callNum);
		}
	}
	fprintf(file,
	        "========================================================================================================================================\n");

	// Print Each
	for (int outi = 0; outi < mCprofileIncnt; ++outi)
	{
		CProfilers * thdCps = &mCprofiles[outi];
		if (thdCps->isValid)
		{
			CProfiler * targetPfArr = thdCps->cprofiles;
			fprintf(file,
			        "----------------------------------------------------------------------------------------------------------------------------------------\n");
			fprintf(file,
			        " Tid  , Name                                             ,           Average  ,           2ndMin   ,           2ndMax   ,          Call \n");

			for (int i = 0; i < thdCps->cprofilesIncnt; ++i)
			{
				if (targetPfArr[i].isValid)
				{
					CProfiler * target = &targetPfArr[i];
					fprintf(file,
					        "%6d,%-50s,%18lf㎲,%18lf㎲,%18lf㎲,%15llu\n",
					        thdCps->tid,
					        target->mTagName,
					        target->GetAverageInMicro(),
					        target->GetMinInMicro(),
					        target->GetMaxInMicro(),
					        target->callNum);
				}
			}
			fprintf(file,
			        "----------------------------------------------------------------------------------------------------------------------------------------\n");
		}
	}

	fclose(file);
}
