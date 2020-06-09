#ifndef MISCASM_H
#define MISCASM_H

class GraphicViewPortClass;

extern "C" int __cdecl Distance_Coord(COORDINATE coord1, COORDINATE coord2);
extern "C" CELL __cdecl Coord_Cell(COORDINATE coord);
extern "C" void __cdecl Fat_Put_Pixel(int x, int y, int color, int siz, GraphicViewPortClass& gpage);
extern "C" long __cdecl Calculate_CRC(void* buffer, long length);

#endif
