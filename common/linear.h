// TiberianDawn.DLL and RedAlert.dll and corresponding source code is free
// software: you can redistribute it and/or modify it under the terms of
// the GNU General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.

// TiberianDawn.DLL and RedAlert.dll and corresponding source code is distributed
// in the hope that it will be useful, but with permitted additional restrictions
// under Section 7 of the GPL. See the GNU General Public License in LICENSE.TXT
// distributed with this program. You should have received a copy of the
// GNU General Public License along with permitted additional restrictions
// with this program. If not, see https://github.com/electronicarts/CnC_Remastered_Collection
#ifndef COMMON_LINEAR_H
#define COMMON_LINEAR_H

int Linear_Blit_To_Linear(void* thisptr,
                          void* dest,
                          int x_pixel,
                          int y_pixel,
                          int dx_pixel,
                          int dy_pixel,
                          int pixel_width,
                          int pixel_height,
                          int trans);
bool Linear_Scale_To_Linear(void* thisptr,
                            void* dest,
                            int src_x,
                            int src_y,
                            int dst_x,
                            int dst_y,
                            int src_width,
                            int src_height,
                            int dst_width,
                            int dst_height,
                            bool trans,
                            char* remap);

#endif
