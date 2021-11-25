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
#include "solectf.h"
#include "soleglobals.h"
#include "solerandom.h"
#include "function.h"

bool Init_Flag_Homes()
{
    bool allocated[20] = {0};

    // For each possible team, assign a waypoint between 4 and 14 as their flag home.
    for (int i = 0; i < 4; ++i) {
        int try_count = 0;

        // Try and randomly assign waypoints from between 4 and 14.
        while (try_count < 100) {
            int waypoint_check = Choose_Random_Val(0, 9);

            if (Waypoint[waypoint_check + 4] != -1 && !allocated[waypoint_check]) {
                FlagHomes[i] = Waypoint[waypoint_check + 4];
                allocated[waypoint_check] = true;
            }

            ++try_count;
        }

        // We failed, try all of the waypoints in range one at a time.
        if (try_count == 100) {
            int waypoint_check = 0;
            while (waypoint_check < 10) {
                if (Waypoint[waypoint_check + 4] != -1 && !allocated[waypoint_check]) {
                    FlagHomes[i] = Waypoint[waypoint_check + 4];
                    allocated[waypoint_check] = true;
                };

                ++waypoint_check;
            }

            // Still failed, not enough waypoints assigned to the map?
            if (waypoint_check == 10) {
                return false;
            }
        }
    }

    return true;
}
