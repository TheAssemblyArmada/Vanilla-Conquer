GraphicViewPort struct
    GVPOffset   DD ? ; offset to virtual viewport
    GVPWidth    DD ? ; width of virtual viewport
    GVPHeight   DD ? ; height of virtual viewport
    GVPXAdd     DD ? ; x mod to get to next line
    GVPXPos     DD ? ; x pos relative to Graphic Buff
    GVPYPos     DD ? ; y pos relative to Graphic Buff
    GVPPitch    DD ? ; modulo of graphic view port
    GVPBuffPtr  DD ? ; ptr to associated Graphic Buff
GraphicViewPort ends
