#ifndef MISCASM_H
#define MISCASM_H

class GraphicViewPortClass;

extern "C" int Distance_Coord(COORDINATE coord1, COORDINATE coord2);
extern "C" CELL Coord_Cell(COORDINATE coord);
extern "C" void Fat_Put_Pixel(int x, int y, int color, int siz, GraphicViewPortClass& gpage);

#endif
