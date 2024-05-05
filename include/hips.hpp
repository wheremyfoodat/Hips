/*

    MIT License

    Copyright (c) 2024 wheremyfoodat/George Poniris

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.

*/

#pragma once
#include <algorithm>
#include <array>
#include <climits>
#include <cstdio>
#include <cstring>
#include <type_traits>
#include <utility>
#include <vector>

namespace Hips {
	using u8 = std::uint8_t;
	using u16 = std::uint16_t;
	using u32 = std::uint32_t;
	using u64 = std::uint64_t;
	using usize = std::size_t;

	using s8 = std::int8_t;
	using s16 = std::int16_t;
	using s32 = std::int32_t;
	using s64 = std::int64_t;

	enum class PatchType : u32 {
		IPS,
		UPS,
		BPS,
	};

	enum class Result : u32 {
		Success = 0,
		InvalidPatch,
		UnknownFormat,
		SizeMismatch,
		ChecksumMismatch,
	};

	namespace Detail {
		// Read "size" bytes, returning 0 if we're going to go out of bounds
		template <typename T = u64, usize size>
		T readBE(const u8* data, usize& offset, usize patchSize) {
			static_assert(std::is_integral<T>() && sizeof(T) >= size);

			offset += size;
			if (offset > patchSize) {
				// We're going to go out of bounds, return 0
				return T(0);
			}

			T ret = T(0);
			int shift = 0;
			// Read byte-by-byte
			for (int i = 0; i < size; i++) {
				ret |= T(data[offset - (i + 1)]) << shift;
				shift += CHAR_BIT;
			}

			return ret;
		}

		template <typename T = u64, usize size>
		T readLE(const u8* data, usize& offset, usize patchSize) {
			static_assert(std::is_integral<T>() && sizeof(T) >= size);

			offset += size;
			if (offset > patchSize) {
				// We're going to go out of bounds, return 0
				return T(0);
			}

			T ret = T(0);
			int shift = (size - 1) * 8;
			// Read byte-by-byte
			for (int i = 0; i < size; i++) {
				ret |= T(data[offset - (i + 1)]) << shift;
				shift -= CHAR_BIT;
			}

			return ret;
		}

		// Formats like UPS and BPS	use run-length encoded integers
		// Regrettably, handling anything > 64 bits is not easy, or particularly worth it
		// Until files start being larger than 18 exabytes that is
		template <typename T = u64>
		T readRunLength(const u8* data, usize& offset, usize patchSize) {
			u64 ret = 0;
			u64 shift = 1;

			while (true) {
				const u64 byte = readLE<u64, 1>(data, offset, patchSize);
				ret += (byte & 0x7F) * shift;

				// If the msb is set then the encoding ends on this byte
				if (byte & 0x80) {
					break;
				}

				shift <<= 7;
				ret += shift;
			}

			return T(ret);
		}

		static u32 crc32(const u8* data, usize length, u32 crc = 0) {
			// 8-bit CRC table
			static constexpr u32 crcTable[] = {
				0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E,
				0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB,
				0xF4D4B551, 0x83D385C7, 0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8,
				0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940,
				0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59, 0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599,
				0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106,
				0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433, 0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB,
				0x086D3D2D, 0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
				0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65, 0x4DB26158, 0x3AB551CE, 0xA3BC0074,
				0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5,
				0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F, 0x5EDEF90E,
				0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
				0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27,
				0x7D079EB1, 0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0,
				0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1,
				0xA6BC5767, 0x3FB506DD, 0x48B2364B, 0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
				0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92,
				0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D, 0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F,
				0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4,
				0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777, 0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
				0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D,
				0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9, 0xBDBDF21C, 0xCABAC28A,
				0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 0xB40BBE37,
				0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
			};

			crc = ~crc;
			for (usize i = 0; i < length; i++) {
				const u8 byte = data[i];
				crc = crcTable[(crc ^ byte) & 0xFF] ^ (crc >> 8);
			}

			return ~crc;
		}
	}  // namespace Detail

	namespace IPS {
		static constexpr usize headerSize = 5;
		// Need at least 5 (header) + 3 (EOF) bytes to be a valid IPS patch
		static constexpr usize minimumPatchSize = headerSize + 3;
		// "EOF" magic string
		static constexpr usize endOfFile = 0x454F46;

		template <typename T = u64, usize size>
		T read(const u8* data, usize& offset, usize patchSize) {
			return Detail::readBE<T, size>(data, offset, patchSize);
		}

		// The size isn't even encoded in the file properly, so we need to parse the file one time first to figure it out...
		static usize getSize(const u8* patch, usize patchSize) {
			usize outputSize = 0;
			usize patchOffset = headerSize;  // Skip header bytes

			while (patchOffset < patchSize) {
				const usize fileOffset = read<usize, 3>(patch, patchOffset, patchSize);

				if (fileOffset == endOfFile) {
					break;
				}

				usize newSize = fileOffset;
				const u16 size = read<u16, 2>(patch, patchOffset, patchSize);
				// RLE encoding
				if (size == 0) {
					const u16 rleSize = read<u16, 2>(patch, patchOffset, patchSize);
					newSize += rleSize;

					patchOffset += 1;  // Skip value field
				} else {
					patchOffset += size;  // Skip data field
					newSize += size;
				}

				outputSize = std::max<usize>(outputSize, newSize);
			}

			if (patchOffset + 3 == patchSize) {
				// Apparently some IPS files have a 3 byte footer with the ROM size after EOF
				const usize actualSize = read<usize, 3>(patch, patchOffset, patchSize);
				outputSize = std::max<usize>(outputSize, actualSize);
			}

			return outputSize;
		}
	};  // namespace IPS

	static std::pair<std::vector<u8>, Result> patchIPS(const u8* data, usize dataSize, const u8* patch, usize patchSize) {
		if (patch == nullptr || patchSize < IPS::minimumPatchSize) [[unlikely]] {
			return {{}, Result::InvalidPatch};
		}

		// Header magic does not match, so the patch is invalid
		if (patch[0] != 'P' || patch[1] != 'A' || patch[2] != 'T' || patch[3] != 'C' || patch[4] != 'H') [[unlikely]] {
			return {{}, Result::InvalidPatch};
		}

		// Copy file to be patched in output buffer
		std::vector<u8> output(IPS::getSize(patch, patchSize));
		std::memcpy(output.data(), data, std::min<u64>(output.size(), dataSize));

		// Skip header
		usize offset = IPS::headerSize;
		while (offset < patchSize) {
			// Read the next record, starting from the 3-byte offset where the patch will be placed in the file to patch
			const usize fileOffset = IPS::read<usize, 3>(patch, offset, patchSize);
			if (fileOffset == IPS::endOfFile) {
				// If we detect EOF, we are done applying the patch
				break;
			}

			// Size of data to copy
			u16 size = IPS::read<u16, 2>(patch, offset, patchSize);
			if (size == 0) {
				// RLE encoding
				u16 rleSize = IPS::read<u16, 2>(patch, offset, patchSize);
				u8 value = IPS::read<u8, 1>(patch, offset, patchSize);

				for (int i = 0; i < rleSize; i++) {
					// Going out of ROM bounds
					if (fileOffset + i >= output.size()) {
						break;
					}

					output[fileOffset + i] = value;
				}
			} else {
				for (int i = 0; i < size; i++) {
					// Going out of ROM bounds
					if (fileOffset + i >= output.size()) {
						break;
					}

					output[fileOffset + i] = IPS::read<u8, 1>(patch, offset, patchSize);
				}
			}
		}

		return {output, Result::Success};
	}

	namespace UPS {
		static constexpr usize headerSize = 4;
		// Need at least 4 (header) + 2 (minimum size for input/output sizes) + crc32s for input file, output file and patch
		static constexpr usize minimumPatchSize = 18;

		template <typename T = u64, usize size>
		T read(const u8* data, usize& offset, usize patchSize) {
			return Detail::readLE<T, size>(data, offset, patchSize);
		}

		template <typename T = u64>
		T readRunLength(const u8* data, usize& offset, usize patchSize) {
			return Detail::readRunLength<T>(data, offset, patchSize);
		}
	}  // namespace UPS

	static std::pair<std::vector<u8>, Result> patchUPS(const u8* data, usize dataSize, const u8* patch, usize patchSize) {
		if (patch == nullptr || patchSize < IPS::minimumPatchSize) [[unlikely]] {
			return {{}, Result::InvalidPatch};
		}

		// Header magic does not match, so the patch is invalid
		if (patch[0] != 'U' || patch[1] != 'P' || patch[2] != 'S' || patch[3] != '1') [[unlikely]] {
			return {{}, Result::InvalidPatch};
		}

		usize patchOffset = UPS::headerSize;
		const u64 inputSize = UPS::readRunLength<u64>(patch, patchOffset, patchSize);
		const u64 outputSize = UPS::readRunLength<u64>(patch, patchOffset, patchSize);

		// The file we're trying to patch is smaller than the input is meant to be, reject it
		if (dataSize < inputSize) {
			return {{}, Result::SizeMismatch};
		}

		std::vector<u8> output(outputSize);
		usize sourceOffset = 0;
		usize outputOffset = 0;

		while (patchOffset < patchSize - 12) {
			u64 length = UPS::readRunLength<u64>(patch, patchOffset, patchSize);

			// Copy length bytes as-is
			while (length > 0 && outputOffset < outputSize) {
				output[outputOffset++] = UPS::read<u8, 1>(data, sourceOffset, dataSize);
				length -= 1;
			}

			while (outputOffset < outputSize) {
				// Patch with XOR until we find the terminating patch value (0x00)
				// Patching with XOR means patches are reversible, by simply applying the patch again
				const u8 sourceValue = UPS::read<u8, 1>(data, sourceOffset, dataSize);
				const u8 patchValue = UPS::read<u8, 1>(patch, patchOffset, patchSize);
				output[outputOffset++] = sourceValue ^ patchValue;

				// We need to check for this after applying the xor patch, otherwise the offset values will be wrong
				if (patchValue == 0) {
					break;
				}
			}
		}

		while (outputOffset < outputSize && sourceOffset < dataSize) {
			// Copy the rest of the bytes
			output[outputOffset++] = UPS::read<u8, 1>(data, sourceOffset, dataSize);
		}

		// Pad rest of the output with 0s
		while (outputOffset < outputSize) {
			output[outputOffset++] = 0;
		}

		const u32 inputCRC = UPS::read<u32, 4>(patch, patchOffset, patchSize);
		const u32 outputCRC = UPS::read<u32, 4>(patch, patchOffset, patchSize);
		const u32 patchCRC = UPS::read<u32, 4>(patch, patchOffset, patchSize);

		if (outputCRC != Detail::crc32(output.data(), output.size())) {
			return {output, Result::ChecksumMismatch};
		}

		return {output, Result::Success};
	}

	namespace BPS {
		static constexpr usize headerSize = 4;
		// Need at least 5 (header) + 3 (source/target/metadata size) + 9 (checksums) bytes to be a valid BPS patch
		static constexpr usize minimumPatchSize = headerSize + 3 + 9;

		template <typename T = u64, usize size>
		T read(const u8* data, usize& offset, usize patchSize) {
			return Detail::readLE<T, size>(data, offset, patchSize);
		}

		template <typename T = u64>
		T readRunLength(const u8* data, usize& offset, usize patchSize) {
			return Detail::readRunLength<T>(data, offset, patchSize);
		}

		namespace Action {
			enum : u32 {
				SourceRead = 0,
				TargetRead = 1,
				SourceCopy = 2,
				TargetCopy = 3,
			};
		}
	}  // namespace BPS

	static std::pair<std::vector<u8>, Result> patchBPS(const u8* data, usize dataSize, const u8* patch, usize patchSize) {
		if (patch == nullptr || patchSize < BPS::minimumPatchSize) [[unlikely]] {
			return {{}, Result::InvalidPatch};
		}

		// Header magic does not match, so the patch is invalid
		if (patch[0] != 'B' || patch[1] != 'P' || patch[2] != 'S' || patch[3] != '1') [[unlikely]] {
			return {{}, Result::InvalidPatch};
		}

		usize patchOffset = BPS::headerSize;
		const u64 inputSize = BPS::readRunLength<u64>(patch, patchOffset, patchSize);
		const u64 outputSize = BPS::readRunLength<u64>(patch, patchOffset, patchSize);
		const u64 metadataSize = BPS::readRunLength<u64>(patch, patchOffset, patchSize);

		// The file we're trying to patch is smaller than the input is meant to be, reject it
		if (dataSize < inputSize) {
			return {{}, Result::SizeMismatch};
		}

		// Copy file to be patched in output buffer
		std::vector<u8> output(outputSize);
		usize sourceOffset = 0;
		usize outputOffset = 0;
		usize outputOffset2 = 0; // Offset used for TargetCopy commands

		while (patchOffset < patchSize - 12) {
			// Each "record" in a BPS patch consists of a VLE word, whose bottom 2 bits are a patching "action" to perform
			// And the top bits are the length of memory to operate on
			const u64 word = BPS::readRunLength<u64>(patch, patchOffset, patchSize);
			const u64 action = (word & 3);
			u64 length = (word >> 2) + 1;

			switch (action) {
				case BPS::Action::SourceRead: {
					while (length > 0 && outputOffset < outputSize && outputOffset < dataSize) {
						output[outputOffset] = data[outputOffset];
						outputOffset += 1;
						length -= 1;
					}
					break;
				}

				case BPS::Action::TargetRead: {
					while (length > 0 && outputOffset < outputSize) {
						output[outputOffset++] = BPS::read<u8, 1>(patch, patchOffset, patchSize);
						length -= 1;
					}
					break;
				}

				case BPS::Action::SourceCopy: {
					const u64 word = BPS::readRunLength<u64>(patch, patchOffset, patchSize);
					const s64 offset = s64(word >> 1);
					sourceOffset += (word & 1) ? -offset : +offset;

					while (length > 0) {
						output[outputOffset++] = data[sourceOffset++];
						length -= 1;
					}
					break;
				}

				case BPS::Action::TargetCopy: {
					const u64 data = BPS::readRunLength<u64>(patch, patchOffset, patchSize);
					const s64 offset = s64(data >> 1);
					outputOffset2 += (data & 1) ? -offset : +offset;

					while (length > 0) {
						output[outputOffset++] = output[outputOffset2++];
						length -= 1;
					}
					break;
				}
			}
		}

		// Pad rest of the output with 0s
		while (outputOffset < outputSize) {
			output[outputOffset++] = 0;
		}

		const u32 inputCRC = BPS::read<u32, 4>(patch, patchOffset, patchSize);
		const u32 outputCRC = BPS::read<u32, 4>(patch, patchOffset, patchSize);
		const u32 patchCRC = BPS::read<u32, 4>(patch, patchOffset, patchSize);

		if (outputCRC != Detail::crc32(output.data(), output.size())) {
			return {output, Result::ChecksumMismatch};
		}

		return {output, Result::Success};
	}

	static std::pair<std::vector<u8>, Result> patch(const u8* data, usize dataSize, const u8* patch, usize patchSize, PatchType type) {
		switch (type) {
			case PatchType::IPS: return patchIPS(data, dataSize, patch, patchSize);
			case PatchType::UPS: return patchUPS(data, dataSize, patch, patchSize);
			case PatchType::BPS: return patchBPS(data, dataSize, patch, patchSize);
			default: return {{}, Result::UnknownFormat};  // Unknown patch format
		}
	}
}  // namespace Hips