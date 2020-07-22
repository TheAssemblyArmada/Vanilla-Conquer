; see tile.h for original struct
IControl_Type struct
    _Width      dw ?
    Height      dw ?
    Count       dw ?
    Allocated   dw ?
    MapWidth    dw ?
    MapHeight   dw ?
    _Size       dd ?
    Icons       dd ?
    Palettes    dd ?
    Remaps      dd ?
    TransFlag   dd ?
    ColorMap    dd ?
    Map         dd ?
IControl_Type ends

include common/stamp.inc

end
