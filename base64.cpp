#include "base64.hpp"

static const std::string key = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                               "abcdefghijklmnopqrstuvwxyz"
                               "0123456789+/";

std::string base64_encode(const std::string &input, bool urlsafe)
{
    std::string ret;
    int j = 0;
    unsigned char array1[3];
    unsigned char array2[4];

    for (int i = 0; i < input.length(); i++)
    {
        array1[j++] = input.at(i);
        
        if (j == 3)
        {
            array2[0] = (array1[0] & 0b11111100) >> 2;
            array2[1] = (array1[0] & 0b00000011) << 4 | (array1[1] & 0b11110000) >> 4;
            array2[2] = (array1[1] & 0b00001111) << 2 | (array1[2] & 0b11000000) >> 6;
            array2[3] = array1[2] & 0b00111111;

            for (int i = 0; i < 4; i++) {
                ret += key.at(array2[i]);
            }
            j = 0;
        }
    }

    if (j) {
        int i = j;
        while (i < 3) {
            array1[i++] = 0x00;
        }

        array2[0] = (array1[0] & 0b11111100) >> 2;
        array2[1] = (array1[0] & 0b00000011) << 4 | (array1[1] & 0b11110000) >> 4;
        array2[2] = (array1[1] & 0b00001111) << 2 | (array1[2] & 0b11000000) >> 6;
        array2[3] = array1[2] & 0b00111111;

        for (i = 0; i < j + 1; i++) {
            ret += key.at(array2[i]);
        }
        while (i < 4) {
            ret += '=';
            i++;
        }
    }

    return ret;
}