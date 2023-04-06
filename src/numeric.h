#pragma once

#include <cmath>

namespace buff {
namespace numeric {
struct PHStorageFloat {
    unsigned char integerPart = 0;
    unsigned char decimalPart = 0;
};

PHStorageFloat smallFloatToStorableParts(const float smallFloat) {
    PHStorageFloat storage;

    float converted = smallFloat * 10;
    converted = roundf(converted);
    int convertedI = (int)converted;
    storage.decimalPart = convertedI % 10;
    storage.integerPart = (convertedI / 10) % 16;

    return storage;
}

float storablePartsToFloat(const PHStorageFloat &storage) {
    float result = 0.0f;
    result += storage.integerPart;
    result += (storage.decimalPart / 10.0);

    return result;
}

unsigned char smallFloatToByte(const float smallFloat) {
    PHStorageFloat parts = smallFloatToStorableParts(smallFloat);
    unsigned char result = 0;

    result += (parts.integerPart << 4);
    result += (parts.decimalPart & 0xF);
    return result;
}

float byteToSmallFloat(const unsigned char encodedSmallFloat) {
    PHStorageFloat parts;
    parts.integerPart = ((encodedSmallFloat & 0xF0) >> 4);
    parts.decimalPart = (encodedSmallFloat & 0xF);
    return storablePartsToFloat(parts);
}

}  // namespace numeric
}  // namespace buff
