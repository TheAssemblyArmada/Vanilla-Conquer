#ifndef MISCASM_H
#define MISCASM_H

class GraphicViewPortClass;

int Distance_Coord(COORDINATE coord1, COORDINATE coord2);
CELL Coord_Cell(COORDINATE coord);
void Fat_Put_Pixel(int x, int y, int color, int siz, GraphicViewPortClass& gpage);

#endif
