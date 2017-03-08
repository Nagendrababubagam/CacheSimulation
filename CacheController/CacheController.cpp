// CacheController.cpp : Defines the entry point for the console application.
#include "stdafx.h"
#include "Helper.h"
#include <complex>
#define complex std::complex<double>
#define CACHE_ALIGN __declspec(align(sizeof(double)))

#define CacheMemory 262144
#define DataBus	4			//32bit Data Bus
#define AddressBus	4			//32bit Data Bus
#define MaxLines 65536		
#define MaxWays 8

#define LineBits(lines) ceil(std::log2(lines))
#define BlockBits(ways)	ceil(std::log2(ways))
uint32_t V[MaxLines][MaxWays];
uint32_t D[MaxLines][MaxWays];
uint32_t LRU[MaxLines][MaxWays];
uint32_t TAG[MaxLines][MaxWays];
uint32_t RRobin[MaxLines];
uint32_t BL_Ways[] = { 1, 2, 4, 8};

// Current Variables per permutation
uint32_t L_Lines;
uint32_t L_Bits;
uint32_t B_Bits;
uint32_t T_Bits;
uint32_t CurrentWays;
uint32_t CurrentBL;
uint32_t CurrentRStrategy;
uint32_t CWriteStartegy;
uint32_t counterVal = 1;

// Function Prototypes
void Radix2FFT(complex data[], uint32_t nPoints, uint32_t nBits);
void Init_Bits(uint32_t BL, uint32_t Ways);
void Init_LRU(void);
void Init_RRobin(void);
uint32_t GetTag(uint32_t Address);
uint32_t GetLineNumber(uint32_t Address);
void Reset_cache(void);
void Flush_cache(void);
void ReadMemory(void *Address, uint32_t Size);
void WriteMemory(void *Address, uint32_t Size);
void Test(void);
void RunPermutationTest(void);

counts Counter = { 0 };
complex CACHE_ALIGN G_data[32768];

int main()
{
	FILE *fp;
	char isTest;
	std::cout << "Would Like to Test Program for Specific Permutation? (Y/N) ";
	std::cin >> isTest;
	std::cout << "\n";
	if (isTest == 'y' || isTest == 'Y')
		RunPermutationTest();
	else
	{
		fopen_s(&fp, "CacheResults.xls", "w+");
		if (fp == NULL)
			fprintf(stderr, "Unable to open file");
		uint32_t TotalPoints = 32768;
		uint32_t i, Bl, Ways, WrStg, ReStg;
		bool Done = false;
		std::cout << ("FFT Test App\r\n");
		
		// time domain: zero-offset real cosine function
		// freq domain: delta fn with zero phase
#define cycles 1 // max cycles is points/2 for Nyquist

		/*for (i = 0; i < points; i++)
			G_data[i] = complex(cos(2.0*3.1416*float(i) / float(points)*cycles), 0.0);*/
		/*
		// time domain: impulse time offset
		// freq domain: constant amplitude of 1
		// phase is zero for zero time offset
		// 1 phase rotation per unit time offset
		#define DELTA_T 0
		for (i = 0; i < points; i++)
		data[i] = complex(0.0, 0.0);
		data[DELTA_T] = complex(1.0, 0.0);
		*/
		std::cout << ("\nCount\t BL\t Ways\t NoLines\t LBits\t BBits\t TBits\t\n\n");
		fprintf(fp, "\t%s\t %s\t %s\t %s\t %s\t %s\t %s\t %s\t %s\t %s\t %s\t %s\t %s\t %s\t %s\t %s\t\n",
			"BL", "Ways", "No.Lines", "Replace Strategy", "Write Strategy", "ReadMemory", "ReadLine", "ReadLineHit",
			"ReadLineDirty", "ReadLineReplace", "WriteMemory", "WriteLine", "WriteLineHit", "WriteLineDirty", "WriteLineReplace",
			"FlushDirtyCount");
		for (ReStg = LRUsed; ReStg <= RoundRobin; ReStg++)
		{
			CurrentRStrategy = ReStg;
			for (WrStg = WriteBack; WrStg <= WriteThroughNonAllocate; WrStg++)
			{
				CWriteStartegy = WrStg;
				for (Bl = 0; Bl < 4; Bl++)					//BL: {1,2,4,8}
				{
					CurrentBL = BL_Ways[Bl];
					for (Ways = 0; Ways < 4; Ways++)				//Ways: {1,2,4,8}
					{
						Counter = { 0 };
						if (CurrentRStrategy == LRUsed)
							Init_LRU();
						else
							Init_RRobin();
						Reset_cache();
						CurrentWays = BL_Ways[Ways];
						Init_Bits(CurrentBL, CurrentWays);
						uint32_t bits = (uint32_t)ceil(log(TotalPoints) / log(2));
						for (i = 0; i < TotalPoints; i++)
							G_data[i] = complex(cos(2.0*3.1416*float(i) / float(TotalPoints)*cycles), 0.0);
						Radix2FFT(G_data, TotalPoints, bits);
						//Test();
						counterVal++;
						Flush_cache();
						fprintf(fp, "\t%d\t %d\t %d\t %d\t %d\t %d\t %d\t %d\t %d\t %d\t %d\t %d\t %d\t %d\t %d\t %d\t\n",
							CurrentBL, CurrentWays, L_Lines, CurrentRStrategy, CWriteStartegy,
							Counter.ReadMemoryCnt, Counter.ReadLineCnt, Counter.ReadLineHitCnt, Counter.ReadLineDirtyCnt, Counter.ReadLineReplaceCnt,
							Counter.WriteMemoryCnt, Counter.WriteLineCnt, Counter.WriteLineHitCnt, Counter.WriteLineDirtyCnt, Counter.WriteLineReplaceCnt,
							Counter.FlushDirtyCount);
					}
				}
			}
		}
		/*for (i = 0; i < points; i++)
		if (data[i].imag() >= 0.0)
		printf("x[%d] = %2.4lf + j%2.4lf\n", i,
		data[i].real(), data[i].imag());
		else
		printf("x[%d] = %2.4lf - j%2.4lf\n", i,
		data[i].real(), -data[i].imag());*/
		fclose(fp);
	}
	std::cout << ("Done!!");
	uint32_t wait;
	std::cin >> wait;
	return 0;
}
void RunPermutationTest(void)
{
	uint32_t i,T_BL, T_Ways, T_RS, T_WS, T_Points;
	std::cout << ("Enter BurstLength in {1,2,4,8} Bytes :\t");
	std::cin >> T_BL;
	std::cout << ("\nEnter Ways(N) in {1,2,4,8} :\t");
	std::cin >> T_Ways;
	std::cout << ("\nEnter WriteStrategy (0-WB, 1-WTA, 2-WTNA) :\t");
	std::cin >> T_WS;
	std::cout << ("\nEnter ReplaceStrategy (0-LRU, 1-RoRobin) :\t");
	std::cin >> T_RS;
	std::cout << ("\nEnter No.of points to run FFT (<= 32768):\t");
	std::cin >> T_Points;
	Counter = { 0 };
	if (CurrentRStrategy == LRUsed)
		Init_LRU();
	else
		Init_RRobin();
	Reset_cache();
	CurrentBL = T_BL;
	CurrentWays = T_Ways;
	std::cout << ("\nCount\t BL\t Ways\t NoLines\t LBits\t BBits\t TBits\t\n\n");
	Init_Bits(CurrentBL, CurrentWays);
	uint32_t bits = (uint32_t)ceil(log(T_Points) / log(2));
	for (i = 0; i < T_Points; i++)
		G_data[i] = complex(cos(2.0*3.1416*float(i) / float(T_Points)*cycles), 0.0);
	Radix2FFT(G_data, T_Points, bits);
	Flush_cache();
	std::cout << ("\n");
	std::cout << "Read Memory Count:\t\t " << (Counter.ReadMemoryCnt) <<"\n";
	std::cout << "Read Line Count:\t\t " << Counter.ReadLineCnt << "\n";
	std::cout << "Read Line Hit Count:\t\t " << Counter.ReadLineHitCnt << "\n";
	std::cout << "Read Line Dirty Count:\t\t " << Counter.ReadLineDirtyCnt << "\n";
	std::cout << "Read Line Replace Count:\t " << Counter.ReadLineReplaceCnt << "\n";
	std::cout << "Write Memory Count:\t\t " << Counter.WriteMemoryCnt << "\n";
	std::cout << "Write Line Count:\t\t " << Counter.WriteLineCnt << "\n";
	std::cout << "Write Line Hit Count:\t\t " << Counter.WriteLineHitCnt << "\n";
	std::cout << "Write Line Dirty Count:\t\t " << Counter.WriteLineDirtyCnt << "\n";
	std::cout << "Write Line Replace Count:\t " << Counter.WriteLineReplaceCnt << "\n";
	std::cout << "Flush Dirty Count:\t\t " << Counter.FlushDirtyCount << "\n";
}
void Radix2FFT(complex data[], uint32_t nPoints, uint32_t nBits)
{
	// cooley-tukey radix-2, twiddle factor
	// adapted from Fortran code from Burrus, 1983
#pragma warning (disable: 4270)
	uint32_t i, j, k, l;
	uint32_t nPoints1, nPoints2;
	complex cTemp, cTemp2;
	ReadMemory(&nPoints, sizeof(nPoints));
	nPoints2 = nPoints;
	WriteMemory(&nPoints2, sizeof(nPoints2));

	WriteMemory(&k, sizeof(k));
	ReadMemory(&k, sizeof(k));
	ReadMemory(&nBits, sizeof(nBits));
	for (k = 1; k <= nBits; k++)
	{
		ReadMemory(&nPoints2, sizeof(nPoints2));
		nPoints1 = nPoints2;
		WriteMemory(&nPoints1, sizeof(nPoints1));

		ReadMemory(&nPoints2, sizeof(nPoints2));
		nPoints2 /= 2;
		WriteMemory(&nPoints2, sizeof(nPoints2));

		// Compute differential angles
		ReadMemory(&nPoints1, sizeof(nPoints1));
		double dTheta = 2 * 3.14159257 / nPoints1;
		WriteMemory(&dTheta, sizeof(dTheta));

		ReadMemory(&dTheta, sizeof(dTheta));
		double dDeltaCos = cos(dTheta);
		WriteMemory(&dDeltaCos, sizeof(dDeltaCos));

		ReadMemory(&dTheta, sizeof(dTheta));
		double dDeltaSin = sin(dTheta);
		WriteMemory(&dDeltaSin, sizeof(dDeltaSin));

		// Initialize angles
		double dCos = 1;
		WriteMemory(&dCos, sizeof(dCos));

		double dSin = 0;
		WriteMemory(&dSin, sizeof(dSin));

		// Perform in-place FFT
		ReadMemory(&j, sizeof(j));
		WriteMemory(&j, sizeof(j));
		ReadMemory(&nPoints2, sizeof(nPoints2));
		for (j = 0; j < nPoints2; j++)
		{
			ReadMemory(&j, sizeof(j));
			i = j;
			WriteMemory(&i, sizeof(i));

			ReadMemory(&i, sizeof(i));
			ReadMemory(&nPoints, sizeof(nPoints));
			while (i < nPoints)
			{
				ReadMemory(&nPoints2, sizeof(nPoints2));
				ReadMemory(&i, sizeof(i));
				l = i + nPoints2;
				WriteMemory(&l, sizeof(l));

				ReadMemory(&data[l], sizeof(data[l]));
				ReadMemory(&data[i], sizeof(data[i]));
				cTemp = data[i] - data[l];
				WriteMemory(&cTemp, sizeof(cTemp));

				ReadMemory(&data[l], sizeof(data[l]));
				ReadMemory(&data[i], sizeof(data[i]));
				cTemp2 = data[i] + data[l];
				WriteMemory(&cTemp2, sizeof(cTemp2));

				ReadMemory(&cTemp2, sizeof(cTemp2));
				data[i] = cTemp2;
				WriteMemory(&data[i], sizeof(data[i]));

				// change it later
				ReadMemory(&cTemp._Val[_RE], sizeof(cTemp._Val[_RE]));
				ReadMemory(&dSin, sizeof(dSin));
				ReadMemory(&cTemp._Val[_IM], sizeof(cTemp._Val[_IM]));
				ReadMemory(&dCos, sizeof(dCos));
				cTemp2 = complex(dCos * cTemp.real() + dSin * cTemp.imag(),
					dCos * cTemp.imag() - dSin * cTemp.real());
				WriteMemory(&cTemp2, sizeof(cTemp2));

				ReadMemory(&cTemp2, sizeof(cTemp2));
				data[l] = cTemp2;
				WriteMemory(&data[l], sizeof(data[l]));

				ReadMemory(&nPoints1, sizeof(nPoints1));
				i += nPoints1;
				WriteMemory(&i, sizeof(i));

				ReadMemory(&i, sizeof(i));
				ReadMemory(&nPoints, sizeof(nPoints));
			}
			ReadMemory(&dCos, sizeof(dCos));
			double dTemp = dCos;
			WriteMemory(&dTemp, sizeof(dTemp));

			ReadMemory(&dDeltaSin, sizeof(dDeltaSin));
			ReadMemory(&dSin, sizeof(dSin));
			ReadMemory(&dDeltaCos, sizeof(dDeltaCos));
			ReadMemory(&dCos, sizeof(dCos));
			dCos = dCos * dDeltaCos - dSin * dDeltaSin;
			WriteMemory(&dCos, sizeof(dCos));

			ReadMemory(&dDeltaCos, sizeof(dDeltaCos));
			ReadMemory(&dSin, sizeof(dSin));
			ReadMemory(&dDeltaSin, sizeof(dDeltaSin));
			ReadMemory(&dTemp, sizeof(dTemp));
			dSin = dTemp * dDeltaSin + dSin * dDeltaCos;
			WriteMemory(&dSin, sizeof(dSin));

			ReadMemory(&j, sizeof(j));
			WriteMemory(&j, sizeof(j));
			ReadMemory(&nPoints2, sizeof(nPoints2));
		}
		ReadMemory(&k, sizeof(k));
		WriteMemory(&k, sizeof(k));
		ReadMemory(&nBits, sizeof(nBits));
	}
	// Convert Bit Reverse Order to Normal Ordering
	j = 0;
	WriteMemory(&j, sizeof(j));

	ReadMemory(&nPoints, sizeof(nPoints));
	nPoints1 = nPoints - 1;
	WriteMemory(&nPoints1, sizeof(nPoints1));

	ReadMemory(&i, sizeof(i));
	WriteMemory(&i, sizeof(i));
	ReadMemory(&nPoints1, sizeof(nPoints1));
	for (i = 0; i < nPoints1; i++)
	{
		ReadMemory(&i, sizeof(i));
		ReadMemory(&j, sizeof(j));
		if (i < j)
		{
			ReadMemory(&data[j], sizeof(data[j]));
			cTemp = data[j];
			WriteMemory(&cTemp, sizeof(cTemp));
			ReadMemory(&data[i], sizeof(data[i]));
			cTemp2 = data[i];
			WriteMemory(&cTemp2, sizeof(cTemp2));
			ReadMemory(&cTemp, sizeof(cTemp));
			data[i] = cTemp;
			WriteMemory(&data[i], sizeof(data[i]));
			ReadMemory(&cTemp2, sizeof(cTemp2));
			data[j] = cTemp2;
			WriteMemory(&data[j], sizeof(data[j]));

			ReadMemory(&i, sizeof(i));
			ReadMemory(&j, sizeof(j));
		}
		ReadMemory(&nPoints2, sizeof(nPoints2));
		k = nPoints / 2;
		WriteMemory(&k, sizeof(k));

		ReadMemory(&k, sizeof(k));
		ReadMemory(&j, sizeof(j));
		while (k <= j)
		{
			ReadMemory(&k, sizeof(k));
			j -= k;
			WriteMemory(&j, sizeof(j));
			ReadMemory(&k, sizeof(k));
			k /= 2;
			WriteMemory(&k, sizeof(k));

			ReadMemory(&k, sizeof(k));
			ReadMemory(&j, sizeof(j));
		}
		ReadMemory(&j, sizeof(j));
		ReadMemory(&k, sizeof(k));
		j += k;
		WriteMemory(&j, sizeof(j));

		ReadMemory(&i, sizeof(i));
		WriteMemory(&i, sizeof(i));
		ReadMemory(&nPoints1, sizeof(nPoints1));
	}
#pragma warning(default: 4270)
}

//Test Program
void Test(void)
{
	uint32_t i;
	uint32_t x[65535];
	WriteMemory(&i, sizeof(i));
	ReadMemory(&i, sizeof(i));
	for (i = 0; i < 65535; i++)
	{
		ReadMemory(&i, sizeof(i));
		WriteMemory(&x[i], sizeof(x[i]));
		x[i] = 0;
		ReadMemory(&i, sizeof(i));
		WriteMemory(&i, sizeof(i));
		ReadMemory(&i, sizeof(i));
	}
}

// Initializes LRU fields of ALL lines.
void Init_LRU()
{
	uint32_t i;
	uint32_t j;
	for (i = 0; i < MaxLines; i++)
	{
		for (j = 0; j < MaxWays; j++)
		{
			LRU[i][j] = j;
		}
	}
}
void Init_RRobin(void)
{
	uint32_t i;
	for (i = 0; i < MaxLines; i++)
	{
		RRobin[i] = 0;
	}
}
// Initializes Bits required based on BL and Ways
void Init_Bits(uint32_t BL, uint32_t Ways)
{
	L_Lines = L_Bits = B_Bits = T_Bits = 0;
	L_Lines = (uint32_t)CacheMemory / (BL*Ways * 4);
	L_Bits = (uint32_t)LineBits(L_Lines);
	B_Bits = (uint32_t)BlockBits(BL*DataBus);
	T_Bits = (uint32_t)(AddressBus * 8 - L_Bits - B_Bits);
	printf("%d\t %d\t %d\t %d\t\t %d\t %d\t %d\t\n", counterVal, BL, Ways, L_Lines, L_Bits, B_Bits, T_Bits);
}
uint32_t GetTag(uint32_t Address)
{
	return (Address >> (AddressBus * 8 - T_Bits));
}
uint32_t GetLineNumber(uint32_t Address)
{
	uint32_t LineNo;
	LineNo = Address << (T_Bits);
	LineNo = LineNo >> (T_Bits + B_Bits);
	return LineNo;
}
// Reads line, looks for a hit or miss, increments relevant counters, updates cache in case of a miss.
void Read_line(uint32_t line, uint32_t Tag)
{
	uint32_t i;
	bool hit = 0;
	int32_t blockToUpdate = -1;

	// Scans the line and it's various ways for hits.
	for (i = 0; i < CurrentWays; i++)
	{
		if (V[line][i] == 1)
		{
			if (TAG[line][i] == Tag)
			{
				// Increments read hit counter, saves the way number for updating LRU.
				Counter.ReadLineHitCnt++;
				hit = 1;
				blockToUpdate = i;
				break;
			}
		}
	}
	// In case of miss, decides "best" block to replace, reads a block in.
	if (!hit)
	{
		if (CurrentRStrategy == LRUsed)
		{
			for (i = 0; i < CurrentWays; i++)
			{
				if (LRU[line][i] == (CurrentWays - 1))
					blockToUpdate = i;
			}
		}
		else
			blockToUpdate = RRobin[line];
		// In case the dirty bit of the block to replace is set, write back to memory.
		if (V[line][blockToUpdate] == 1)
		{
			if (D[line][blockToUpdate] == 1)
			{
				//Counter.WriteMemoryCnt++;
				Counter.ReadLineDirtyCnt++;
				D[line][blockToUpdate] = 0;
			}
		}
		// Read Memory to cache	
		Counter.ReadLineReplaceCnt++;
		TAG[line][blockToUpdate] = Tag;
		V[line][blockToUpdate] = 1;
		if (CWriteStartegy == WriteBack)
			D[line][blockToUpdate] = 1;
	}
	// Updating LRU in all cases
	if (CurrentRStrategy == LRUsed)
	{
		for (i = 0; i < CurrentWays; i++)
		{
			if (LRU[line][i] < LRU[line][blockToUpdate])
			{
				LRU[line][i] = LRU[line][i] + 1;
			}
		}
		LRU[line][blockToUpdate] = 0;
	}
	// Updating RoundRobin if block is replaced
	else
	{
		if (!hit)
			RRobin[line] = (RRobin[line] + 1 > CurrentWays - 1) ? 0 : RRobin[line];
	}
}

void Write_line(uint32_t line, uint32_t Tag)
{
	uint32_t i;
	bool hit = 0;
	int32_t blockToUpdate = -1;
	for (i = 0; i < CurrentWays; i++)                    //searches various ways of each line.
	{
		if (V[line][i] == 1)                      //in case of a hit, depending on the write strategy, increments counters.
		{
			if (TAG[line][i] == Tag)
			{
				hit = true;
				blockToUpdate = i;
				break;
			}
		}
	}
	// In case of a miss, depending on the write strategy, increments counters.
	if (!hit && (CWriteStartegy == WriteThroughAllocate || CWriteStartegy == WriteBack))
	{
		if (CurrentRStrategy == LRUsed)
		{
			for (i = 0; i < CurrentWays; i++)
			{
				if (LRU[line][i] == (CurrentWays - 1))
					blockToUpdate = i;
			}
		}
		else
			blockToUpdate = RRobin[line];
		// If block to update is Dirty -> Write that block to memory
		if (D[line][blockToUpdate] == 1)
		{
			//Counter.WriteMemoryCnt++;
			Counter.WriteLineDirtyCnt++;
			D[line][blockToUpdate] = 0;
		}
		// Bring Block from memory now to cache
		// Counter.ReadMemoryCnt++;
		Counter.WriteLineReplaceCnt++;
		TAG[line][blockToUpdate] = Tag;
		V[line][blockToUpdate] = 1;
		/*if (CWriteStartegy == WriteBack)
		D[line][blockToUpdate] = 1;*/
	}
	if (CWriteStartegy == WriteThroughAllocate || CWriteStartegy == WriteBack || (hit && CWriteStartegy == WriteThroughNonAllocate))
	{
		// counter to increment??
		if(hit)
			Counter.WriteLineHitCnt++;
		if(CWriteStartegy == WriteBack)
			D[line][blockToUpdate] = 1;
	}
	if (CWriteStartegy == WriteThroughAllocate || CWriteStartegy == WriteThroughNonAllocate)
	{
		Counter.WriteThroughMemoryCnt++;
	}

	//Update LRU and RounRobin
	if (CurrentRStrategy == LRUsed)
	{
		for (i = 0; i < CurrentWays; i++)
		{
			if (CWriteStartegy == 0 || CWriteStartegy == 1)          //for WriteBack and WriteThrough, updates LRU.
			{
				if (LRU[line][i] < LRU[line][blockToUpdate])
				{
					LRU[line][i] = LRU[line][i] + 1;
				}
			}
		}
		LRU[line][blockToUpdate] = 0;
	}
	else
	{
		if (!hit)
			RRobin[line] = (RRobin[line] + 1 > CurrentWays - 1) ? 0 : RRobin[line];
	}
}

void ReadMemory(void *Address, uint32_t Size)
{
	uint32_t i;
	uint32_t Address_32bit = (uint32_t)Address;
	uint32_t last_line = -1;
	uint32_t LineNo = -1;
	uint32_t CTag = -1;
	Counter.ReadMemoryCnt++;
	for (i = 0; i < Size; i++)
	{
		LineNo = GetLineNumber(Address_32bit);
		CTag = GetTag(Address_32bit);
		if (LineNo != last_line)
		{
			last_line = LineNo;
			Read_line(LineNo, CTag);
			Counter.ReadLineCnt++;
		}
		Address_32bit = Address_32bit + 1;
	}
}
void WriteMemory(void *Address, uint32_t Size)
{
	uint32_t i;
	uint32_t Address_32bit = (uint32_t)Address;
	uint32_t last_line = -1;
	uint32_t LineNo = -1;
	uint32_t CTag = -1;
	Counter.WriteMemoryCnt++;
	for (i = 0; i < Size; i++)
	{
		LineNo = GetLineNumber(Address_32bit);
		CTag = GetTag(Address_32bit);

		if (LineNo != last_line)
		{
			last_line = LineNo;
			Write_line(LineNo, CTag);
			Counter.WriteLineCnt++;	
		}
		Address_32bit = Address_32bit + 1;
	}
}
// Flushes all dirty bits by writing everything to memory and increments flush counter.
void Flush_cache()
{
	uint32_t i, j;
	for (i = 0; i<MaxLines; i++)
	{
		for (j = 0; j<MaxWays; j++)
		{
			if (D[i][j] == 1)
			{
				Counter.FlushDirtyCount++;
				D[i][j] = 0;
			}
		}
	}
}
// Resets all valid bits.
void Reset_cache()
{
	uint32_t i, j;
	for (i = 0; i<MaxLines; i++)
	{
		for (j = 0; j < MaxWays; j++)
			V[i][j] = 0;
	}
}