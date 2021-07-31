#include <iostream>
#include <fstream>
#include <array>
#include <vector>
#include <algorithm>
#include <cassert>

typedef uint8_t byte;

#pragma pack(push, 1)

struct SysExDt1Header
{
	byte ExclusiveStatus = 0xF0;
	byte ManufactorId = 0x41;
	byte DeviceId = 0x00;
	byte ModelId = 0x14;
	byte CommandId = 0x12;

	byte AddressMsb;
	byte Address;
	byte AddressLsb;
};

struct SysExFooter
{
	byte Checksum = 0x00;
	byte EndOfExclusive = 0xF7;
};

struct SysEx
{
	SysExDt1Header Header;
	std::vector<byte> Content;
	SysExFooter Footer;
};

// 4-4
// Length = 64
struct Partial_F
{
	byte WgPitchCoarse;
	byte WgPitchFine;
	byte WgPitchKeyfollow;
	byte WgModLfoMode;
	byte WgModPEnvMode;
	byte WgModBendMode;
	byte WgWaveForm;
	byte WgPcmWaveNo;
	byte WgPulseWidth;
	byte WgPwVelocityRange;
	byte WgPwLfoSelect;
	byte WgPwLfoDepth;
	byte WgPwAftertouchRange;
	byte TvfCutoffFrequency;
	byte TvfResonance;
	byte TvfKeyfollow;

	byte TvfBiasPoint;
	byte TvfBiasLevel;
	byte TvfEnvDepth;
	byte TvfEnvVelocityRange;
	byte TvfEnvDepthKeyfollow;
	byte TvfEnvTimeKeyfollow;
	byte TvfEnvTime1;
	byte TvfEnvTime2;
	byte TvfEnvTime3;
	byte TvfEnvTime4;
	byte TvfEnvTime5;
	byte TvfEnvLevel1;
	byte TvfEnvLevel2;
	byte TvfEnvLevel3;
	byte TvfEnvSustainLevel;
	byte TvfEnvEndLevel;

	//byte TvfMod
};

struct Partial
{
	byte arr[64];
};

// 4-5
struct Common
{
	byte ToneName[10];
	byte arr[54];
};

// 4-6
struct Patch
{
	byte PatchName[18];
	byte KeyMode;
	byte arr[64 - 19];
};

#pragma pack(pop)

template<int size>
void ConvertToAsciiBytes(byte(&x)[size])
{
	static char nameTable[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890-";
	for (int i = 0; i < size; i++)
	{
		assert(x[i] < strlen(nameTable));
		x[i] = nameTable[x[i]];
	}
}

template<int size>
void ConvertToD50CharBytes(byte(&x)[size])
{
	static char nameTable[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890-";
	for (int i = 0; i < size; i++)
	{
		auto ch = x[i];
		auto outCh = 0;
		if (ch == ' ')
		{
			outCh = 0;
		}
		else if (ch >= 'A' && ch <= 'Z')
		{
			outCh = (ch - 'A') + 1;
		}
		else if (ch >= 'a' && ch <= 'z')
		{
			outCh = (ch - 'a') + 27;
		}
		else if (ch >= '1' && ch <= '9')
		{
			outCh = (ch - '1') + 53;
		}
		else if (ch == '0')
		{
			outCh = 62;
		}
		else if (ch == '-')
		{
			outCh = 63;
		}
		else
		{
			outCh = 63;
		}

		x[i] = outCh;
	}
}

template<int size>
std::string ConvertToAsciiString(byte (&x)[size])
{
	static char nameTable[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890-";
	char out[size + 1] = { 0, };
	for (int i = 0; i < size; i++)
	{
		assert(x[i] < strlen(nameTable));
		out[i] = nameTable[x[i]];
	}
	return out;
}

void ConvertToVstiKeyMode(byte& keyMode)
{
	// There's no SEPARATE mode in the VSTi
	if (keyMode == 8)
		keyMode = 2;
	else if (keyMode >= 3)
		keyMode--;
}

void ConvertToD50KeyMode(byte& keyMode)
{
	// There's no SEPARATE mode in the VSTi
	if (keyMode >= 3)
		keyMode++;
}

struct SyxPatch
{
	Partial UpperPartial1;
	Partial UpperPartial2;
	Common UpperCommon;
	Partial LowerPartial1;
	Partial LowerPartial2;
	Common LowerCommon;
	Patch Patch;
};

struct SyxReverb
{
	byte arr[376];
};

struct BinPatch
{
	byte ToneName[20];
	Partial UpperPartial1;
	Partial UpperPartial2;
	Partial LowerPartial1;
	Partial LowerPartial2;
	Common UpperCommon;
	Common LowerCommon;
	Patch Patch;
};

const int BankSize = 64;

const int PartialLength = 64;
const int CommonLength = 64;
const int PatchLength = 64;

const int CommonNameLength = 10;
const int PatchNameLength = 18;

const int ReverbLength = 376;

const int BinHeaderLength = 0x16;
const int BinPatchLength = 468;

const int SyxChunkMaxContentLength = 256;
const int SyxChunkMaxLength = SyxChunkMaxContentLength + sizeof(SysExDt1Header) + sizeof(SysExFooter);

std::vector<uint8_t> ReadFile(const std::string& fileName)
{
	std::basic_ifstream<byte> is(fileName, std::basic_ifstream<byte>::binary);

	std::vector<byte> buffer(std::istreambuf_iterator<byte>(is), {});

	return buffer;
}

template<typename ... Args>
std::string string_format(const std::string& format, Args ... args)
{
	int size_s = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
	if (size_s <= 0) { throw std::runtime_error("Error during formatting."); }
	auto size = static_cast<size_t>(size_s);
	auto buf = std::make_unique<char[]>(size);
	std::snprintf(buf.get(), size, format.c_str(), args ...);
	return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}

void DumpSyx(const std::string& fileName, const std::vector<byte>& syx)
{
	std::ofstream os(fileName);
	int base = 0;
	for (int i = 0; i < syx.size(); i++)
	{
		if (syx[i] == 0xf7)
		{
			// base .. i
			for (int x = base; x < base + std::min((int)syx.size() - base, SyxChunkMaxLength); x++)
			{
				std::string a = string_format(" %02X", syx[x]);
				os.write(a.c_str(), a.size());
			}
			base = i + 1;
			std::string eol = "\n";
			os.write(eol.c_str(), eol.size());
		}
	}
}

void DumpBin(const std::string& fileName, const std::vector<byte>& bin)
{
	std::ofstream os(fileName);
	int base = BinHeaderLength;
	for (int i = 0; i < (bin.size() - BinHeaderLength) / BinPatchLength; i++)
	{
		int base = i * BinPatchLength + BinHeaderLength;
		for (int x = base; x < base + BinPatchLength; x++)
		{
			std::string a = string_format(" %02X", bin[x]);
			os.write(a.c_str(), a.size());
		}
		std::string eol = "\n";
		os.write(eol.c_str(), eol.size());
	}
}

void ReadSyx(const std::vector<byte>& syx, std::vector<SyxPatch>& syxPatches, std::vector<SyxReverb>& syxReverbs)
{
	std::vector<byte> patchMem;
	std::vector<byte> reverbMem;

	int base = 0;
	for (int i = 0; i < syx.size(); i++)
	{
		if (syx[i] == 0xf7)
		{
			int length = std::min((int)syx.size() - base, SyxChunkMaxLength);
			int contentLength = length - sizeof(SysExDt1Header) - sizeof(SysExFooter);

			int contentBase = base + sizeof(SysExDt1Header);

			SysExDt1Header header;
			memcpy(&header, &syx[base], sizeof(header));

			assert(header.ExclusiveStatus == 0xF0 &&
				header.ManufactorId == 0x41 &&
				header.DeviceId == 0x00 &&
				header.ModelId == 0x14 &&
				header.CommandId == 0x12);

			if (header.AddressMsb > 0x03 || header.AddressMsb == 0x03 && header.Address >= 0x60)
			{
				reverbMem.reserve(reverbMem.size() + contentLength);
				reverbMem.insert(reverbMem.end(), syx.begin() + contentBase, syx.begin() + contentBase + contentLength);
			}
			else
			{
				patchMem.reserve(patchMem.size() + contentLength);
				patchMem.insert(patchMem.end(), syx.begin() + contentBase, syx.begin() + contentBase + contentLength);
			}

			base = i + 1;
		}
	}

	assert(patchMem.size() == 0x7000);
	assert(reverbMem.size() == 0x1780);

	for (int i = 0; i < patchMem.size() / sizeof(SyxPatch); i++)
	{
		SyxPatch patch;

		memcpy(&patch, &patchMem[i * sizeof(SyxPatch)], sizeof(SyxPatch));

		printf("Patch %d: %s\n", i + 1, ConvertToAsciiString(patch.Patch.PatchName).c_str());

		syxPatches.emplace_back(patch);
	}

	for (int i = 0; i < reverbMem.size() / sizeof(SyxReverb); i++)
	{
		SyxReverb reverb;

		memcpy(&reverb, &reverbMem[i * sizeof(SyxReverb)], sizeof(SyxReverb));

		syxReverbs.emplace_back(reverb);
	}
}

void ReadBin(const std::vector<byte>& bin, std::vector<BinPatch>& binPatches)
{
	assert(bin.size() == BinHeaderLength + BinPatchLength * BankSize);

	std::vector<byte> patchMem;
	for (int i = 0; i < (bin.size() - BinHeaderLength) / BinPatchLength; i++)
	{
		int base = i * BinPatchLength + BinHeaderLength;

		patchMem.reserve(patchMem.size() + BinPatchLength);
		patchMem.insert(patchMem.end(), bin.begin() + base, bin.begin() + base + BinPatchLength);
	}

	assert(patchMem.size() == 0x7500);

	for (int i = 0; i < patchMem.size() / sizeof(BinPatch); i++)
	{
		BinPatch patch;

		memcpy(&patch, &patchMem[i * sizeof(BinPatch)], sizeof(BinPatch));

		byte name[18 + 1] = { 0, };
		memcpy(name, patch.Patch.PatchName, sizeof(patch.Patch.PatchName));

		printf("Patch %d: %s\n", i + 1, name);

		binPatches.emplace_back(patch);
	}
}

// NOTE: There's no reverb section in the bin format
void ConvertSyxToBin(const std::vector<SyxPatch>& syxPatches, std::vector<BinPatch>& binPatches)
{
	for (int i = 0; i < syxPatches.size(); i++)
	{
		const SyxPatch& syxPatch = syxPatches[i];
		BinPatch binPatch;

		binPatch.UpperPartial1 = syxPatch.UpperPartial1;
		binPatch.UpperPartial2 = syxPatch.UpperPartial2;
		binPatch.LowerPartial1 = syxPatch.LowerPartial1;
		binPatch.LowerPartial2 = syxPatch.LowerPartial2;

		binPatch.UpperCommon = syxPatch.UpperCommon;
		binPatch.LowerCommon = syxPatch.LowerCommon;
		binPatch.Patch = syxPatch.Patch;

		ConvertToAsciiBytes(binPatch.UpperCommon.ToneName);
		ConvertToAsciiBytes(binPatch.LowerCommon.ToneName);
		ConvertToAsciiBytes(binPatch.Patch.PatchName);

		memset(binPatch.ToneName, 0, sizeof(binPatch.ToneName));
		memcpy(binPatch.ToneName, binPatch.Patch.PatchName, sizeof(binPatch.Patch.PatchName));

		ConvertToVstiKeyMode(binPatch.Patch.KeyMode);

		// Signature?
		binPatch.Patch.arr[44 - 19] = 0x03;
		binPatch.Patch.arr[45 - 19] = 0x0f;
		binPatch.Patch.arr[46 - 19] = 0x0d;

		binPatches.emplace_back(binPatch);
	}
}

void ConvertBinToSyx(const std::vector<BinPatch>& binPatches, std::vector<SyxPatch>& syxPatches)
{
	for (int i = 0; i < binPatches.size(); i++)
	{
		const BinPatch& binPatch = binPatches[i];
		SyxPatch syxPatch;

		syxPatch.UpperPartial1 = binPatch.UpperPartial1;
		syxPatch.UpperPartial2 = binPatch.UpperPartial2;
		syxPatch.LowerPartial1 = binPatch.LowerPartial1;
		syxPatch.LowerPartial2 = binPatch.LowerPartial2;

		syxPatch.UpperCommon = binPatch.UpperCommon;
		syxPatch.LowerCommon = binPatch.LowerCommon;
		syxPatch.Patch = binPatch.Patch;

		ConvertToD50CharBytes(syxPatch.UpperCommon.ToneName);
		ConvertToD50CharBytes(syxPatch.LowerCommon.ToneName);
		ConvertToD50CharBytes(syxPatch.Patch.PatchName);

		ConvertToD50KeyMode(syxPatch.Patch.KeyMode);

		syxPatch.Patch.arr[44 - 19] = 0x00;
		syxPatch.Patch.arr[45 - 19] = 0x00;
		syxPatch.Patch.arr[46 - 19] = 0x00;

		syxPatches.emplace_back(syxPatch);
	}
}

void WriteBin(const std::string fileName, const std::vector<BinPatch>& binPatches)
{
	int outLength = binPatches.size() * sizeof(BinPatch);
	std::basic_ofstream<byte> os(fileName, std::basic_ofstream<byte>::binary);
	byte binHeader[] = "KoaBankFile00003PG-D50";
	os.write(binHeader, sizeof(binHeader) - 1);
	os.write(reinterpret_cast<const byte*>(binPatches.data()), outLength);
}

byte CalculateChecksum(std::vector<byte>::const_iterator iter, int length)
{
	byte checksum = 0;
	for (int i = 0; i < length; i++, ++iter)
	{
		checksum += *iter;
	}

	return checksum;
}

byte ConvertToRolandChecksum(byte sum)
{
	sum = 0x80 - (sum % 0x80);
	if (sum == 0x80)
		sum = 0;
	return sum;
}

void CalculateAddress(int base, int offset, byte& h, byte& m, byte& l)
{
	int x = base + offset;

	h = (byte)((x >> 14) & 0x7f);
	m = (byte)((x >> 7) & 0x7f);
	l = (byte)(x & 0x7f);
}

void WriteSyx(const std::string fileName, const std::vector<SyxPatch>& syxPatches)
{
	assert(syxPatches.size() <= 64);

	std::basic_ofstream<byte> os(fileName, std::basic_ofstream<byte>::binary);

	int outLength = syxPatches.size() * sizeof(SyxPatch);
	const byte* bytes = reinterpret_cast<const byte*>(syxPatches.data());
	std::vector<byte> rawVec(bytes, bytes + outLength);
	for (int i = 0; i < outLength; i += SyxChunkMaxContentLength)
	{
		int remainingLength = (outLength - i);
		int currentChunkLen = std::min(remainingLength, SyxChunkMaxContentLength);

		SysExDt1Header header;
		SysExFooter footer;

		CalculateAddress(0x8000, i, header.AddressMsb, header.Address, header.AddressLsb);

		footer.Checksum = CalculateChecksum(rawVec.begin() + i, currentChunkLen);
		footer.Checksum += header.AddressLsb;
		footer.Checksum += header.Address;
		footer.Checksum += header.AddressMsb;
		footer.Checksum = ConvertToRolandChecksum(footer.Checksum);

		os.write(reinterpret_cast<const byte*>(&header), sizeof(header));
		os.write(bytes + i, currentChunkLen);
		os.write(reinterpret_cast<const byte*>(&footer), sizeof(footer));
	}
}

std::string ToLower(const std::string& str)
{
	std::string out = str;
	std::transform(out.begin(), out.end(), out.begin(), [](char c) { return std::tolower(c); });
	return out;
}

bool EndsWith(std::string const& fullString, std::string const& ending)
{
	if (fullString.length() >= ending.length())
	{
		return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
	}
	else
	{
		return false;
	}
}

int main(int argc, char** argv)
{
	printf("Roland D-50 SYX <-> BIN Converter\n\n");

	if (argc < 2)
	{
		printf("Usage: exec -i input -o output\n");
		return 0;
	}

	bool inputIsSet = false;
	bool outputIsSet = false;
	std::string inputFileName;
	std::string outputFileName;
	for (int i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i], "-i") || !strcmp(argv[i], "--input") && argc > i + 1)
		{
			inputFileName = argv[i + 1];
			inputIsSet = true;
		}
		else if (!strcmp(argv[i], "-o") || !strcmp(argv[i], "--output"))
		{
			outputFileName = argv[i + 1];
			outputIsSet = true;
		}
	}

	// validate
	if (!inputIsSet)
	{
		printf("No input file specified\n");
		return 0;
	}

	if (!outputIsSet)
	{
		printf("No output file specified\n");
		return 0;
	}

	bool fromSyx = false;

	const std::string syxExt = ".syx";
	const std::string binExt = ".bin";
	if (EndsWith(ToLower(inputFileName), syxExt) &&
		EndsWith(ToLower(outputFileName), binExt))
	{
		fromSyx = true;
	}
	else if (EndsWith(ToLower(inputFileName), binExt) &&
		EndsWith(ToLower(outputFileName), syxExt))
	{
		fromSyx = false;
	}
	else
	{
		printf("Invalid format\n");
		return -1;
	}

	if (fromSyx)
	{
		std::vector<byte> syx = ReadFile(inputFileName);
		DumpSyx("dump_syx.txt", syx);

		std::vector<SyxPatch> syxPatches;
		std::vector<SyxReverb> syxReverbs;

		ReadSyx(syx, syxPatches, syxReverbs);

		std::vector<BinPatch> binPatches;
		ConvertSyxToBin(syxPatches, binPatches);

		WriteBin(outputFileName, binPatches);
	}
	else
	{
		std::vector<byte> bin = ReadFile(inputFileName);
		DumpBin("dump_bin.txt", bin);

		std::vector<BinPatch> binPatches;
		ReadBin(bin, binPatches);

		std::vector<SyxPatch> syxPatches;
		ConvertBinToSyx(binPatches, syxPatches);

		WriteSyx(outputFileName, syxPatches);
	}

	printf("\nConvert success\n");

	return 0;
}
