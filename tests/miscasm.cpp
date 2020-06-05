// tested functions
int __cdecl calcx_A(signed short param1, short distance);
int __cdecl calcy_A(signed short param1, short distance);
int __cdecl calcx(signed short param1, short distance);
int __cdecl calcy(signed short param1, short distance);

#include <iostream>

int main(int argc, char** argv)
{
    for (signed short i = -1000; i < 1000; i++) {
        for (short distance = -100; i < 100; i++) {
            if (calcx_A(i, distance) != calcx(i, distance)) {
                std::cerr << "calcx test failed with arguments" << i << distance << std::endl;
                return -1;
            }
        }
    }
}
