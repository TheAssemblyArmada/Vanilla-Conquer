//
// Copyright 2020 Electronic Arts Inc.
//
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

#define TXT_NONE                              0   //
#define TXT_CREDIT_FORMAT                     1   // %3d.%02d
#define TXT_TIME_FORMAT_HOURS                 2   // Time:%02d:%02d:%02d
#define TXT_TIME_FORMAT_NO_HOURS              3   // Time:%02d:%02d
#define TXT_BUTTON_SELL                       4   // Sell
#define TXT_SELL                              5   // Sell Structure
#define TXT_BUTTON_REPAIR                     6   // Repair
#define TXT_YOU                               7   // You:
#define TXT_ENEMY                             8   // Enemy:
#define TXT_BUILD_DEST                        9   // Buildings Destroyed By
#define TXT_UNIT_DEST                         10  // Units Destroyed By
#define TXT_TIB_HARV                          11  // Ore Harvested By
#define TXT_SCORE_1                           12  // Score: %d
#define TXT_YES                               13  // Yes
#define TXT_NO                                14  // No
#define TXT_SCENARIO_WON                      15  // Mission Accomplished
#define TXT_SCENARIO_LOST                     16  // Mission Failed
#define TXT_START_NEW_GAME                    17  // Start New Game
#define TXT_INTRO                             18  // Intro & Sneak Peek
#define TXT_CANCEL                            19  // Cancel
#define TXT_ROCK                              20  // Rock
#define TXT_CIVILIAN                          21  // Civilian
#define TXT_JP                                22  // Containment Team
#define TXT_OK                                23  // OK
#define TXT_TREE                              24  // Tree
#define TXT_LEFT                              25  // 
#define TXT_RIGHT                             26  // 
#define TXT_UP                                27  // 
#define TXT_DOWN                              28  // 
#define TXT_CLEAR                             29  // Clear
#define TXT_WATER                             30  // Water
#define TXT_ROAD                              31  // Road
#define TXT_SLOPE                             32  // Slope
#define TXT_PATCH                             33  // Patch
#define TXT_RIVER                             34  // River
#define TXT_LOAD_MISSION                      35  // Load Mission
#define TXT_SAVE_MISSION                      36  // Save Mission
#define TXT_DELETE_MISSION                    37  // Delete Mission
#define TXT_LOAD_BUTTON                       38  // Load
#define TXT_SAVE_BUTTON                       39  // Save
#define TXT_DELETE_BUTTON                     40  // Delete
#define TXT_GAME_CONTROLS                     41  // Game Controls
#define TXT_SOUND_CONTROLS                    42  // Sound Controls
#define TXT_RESUME_MISSION                    43  // Resume Mission
#define TXT_VISUAL_CONTROLS                   44  // Visual Controls
#define TXT_QUIT_MISSION                      45  // Abort Mission
#define TXT_EXIT_GAME                         46  // Exit Game
#define TXT_OPTIONS                           47  // Options
#define TXT_SQUISH                            48  // Squish mark
#define TXT_CRATER                            49  // Crater
#define TXT_SCORCH                            50  // Scorch Mark
#define TXT_BRIGHTNESS                        51  // BRIGHTNESS:
#define TXT_MUSIC                             52  // Music Volume
#define TXT_VOLUME                            53  // Sound Volume
#define TXT_TINT                              54  // Tint:
#define TXT_CONTRAST                          55  // Contrast:
#define TXT_SPEED                             56  // Game Speed:
#define TXT_SCROLLRATE                        57  // Scroll Rate:
#define TXT_COLOR                             58  // Color:
#define TXT_RETURN_TO_GAME                    59  // Return to game
#define TXT_ENEMY_SOLDIER                     60  // Enemy Soldier
#define TXT_ENEMY_VEHICLE                     61  // Enemy Vehicle
#define TXT_ENEMY_STRUCTURE                   62  // Enemy Structure
#define TXT_LTANK                             63  // Light Tank
#define TXT_MTANK                             64  // Heavy Tank
#define TXT_MTANK2                            65  // Medium Tank
#define TXT_HTANK                             66  // Mammoth Tank
#define TXT_SAM                               67  // SAM Site
#define TXT_JEEP                              68  // Ranger
#define TXT_TRANS                             69  // Chinook Helicopter
#define TXT_HARVESTER                         70  // Ore Truck
#define TXT_ARTY                              71  // Artillery
#define TXT_E1                                72  // Rifle Infantry
#define TXT_E2                                73  // Grenadier
#define TXT_E3                                74  // Rocket Soldier
#define TXT_E4                                75  // Flamethrower
#define TXT_HELI                              76  // Longbow Helicopter
#define TXT_ORCA                              77  // Hind
#define TXT_APC                               78  // APC
#define TXT_GUARD_TOWER                       79  // Guard Tower
#define TXT_COMMAND                           80  // Radar Dome
#define TXT_HELIPAD                           81  // Helipad
#define TXT_AIRSTRIP                          82  // Airfield
#define TXT_STORAGE                           83  // Ore Silo
#define TXT_CONST_YARD                        84  // Construction Yard
#define TXT_REFINERY                          85  // Ore Refinery
#define TXT_CIV1                              86  // Church
#define TXT_CIV2                              87  // Han's and Gretel's
#define TXT_CIV3                              88  // Hewitt's Manor
#define TXT_CIV4                              89  // Ricktor's House
#define TXT_CIV5                              90  // Gretchin's House
#define TXT_CIV6                              91  // The Barn
#define TXT_CIV7                              92  // Damon's pub
#define TXT_CIV8                              93  // Fran's House
#define TXT_CIV9                              94  // Music Factory
#define TXT_CIV10                             95  // Toymaker's
#define TXT_CIV11                             96  // Ludwig's House
#define TXT_CIV12                             97  // Haystacks
#define TXT_CIV13                             98  // Haystack
#define TXT_CIV14                             99  // Wheat Field
#define TXT_CIV15                             100 // Fallow Field
#define TXT_CIV16                             101 // Corn Field
#define TXT_CIV17                             102 // Celery Field
#define TXT_CIV18                             103 // Potato Field
#define TXT_CIV20                             104 // Sala's House
#define TXT_CIV21                             105 // Abdul's House
#define TXT_CIV22                             106 // Pablo's Wicked Pub
#define TXT_CIV23                             107 // Village Well
#define TXT_CIV24                             108 // Camel Trader
#define TXT_CIV25                             109 // Church
#define TXT_CIV26                             110 // Ali's House
#define TXT_CIV27                             111 // Trader Ted's
#define TXT_CIV28                             112 // Menelik's House
#define TXT_CIV29                             113 // Prestor John's House
#define TXT_CIV30                             114 // Village Well
#define TXT_CIV31                             115 // Witch Doctor's Hut
#define TXT_CIV32                             116 // Rikitikitembo's Hut
#define TXT_CIV33                             117 // Roarke's Hut
#define TXT_CIV34                             118 // Mubasa's Hut
#define TXT_CIV35                             119 // Aksum's Hut
#define TXT_CIV36                             120 // Mambo's Hut
#define TXT_CIV37                             121 // The Studio
#define TXT_CIVMISS                           122 // Technology Center
#define TXT_TURRET                            123 // Turret
#define TXT_GUNBOAT                           124 // Gunboat
#define TXT_MCV                               125 // Mobile Construction Vehicle
#define TXT_POWER                             126 // Power Plant
#define TXT_ADVANCED_POWER                    127 // Advanced Power Plant
#define TXT_HOSPITAL                          128 // Hospital
#define TXT_BARRACKS                          129 // Barracks
#define TXT_PUMP                              130 // Oil Pump
#define TXT_TANKER                            131 // Oil Tanker
#define TXT_SANDBAG_WALL                      132 // Sandbags
#define TXT_CYCLONE_WALL                      133 // Chain Link Fence
#define TXT_BRICK_WALL                        134 // Concrete Wall
#define TXT_BARBWIRE_WALL                     135 // Barbwire Fence
#define TXT_WOOD_WALL                         136 // Wood Fence
#define TXT_WEAPON_FACTORY                    137 // War Factory
#define TXT_AGUARD_TOWER                      138 // Advanced Guard Tower
#define TXT_BIO_LAB                           139 // Bio-Research Laboratory
#define TXT_FIX_IT                            140 // Service Depot
#define TXT_TAB_SIDEBAR                       141 // Sidebar
#define TXT_TAB_BUTTON_CONTROLS               142 // Options
#define TXT_TAB_BUTTON_DATABASE               143 // Database
#define TXT_SHADOW                            144 // Unrevealed Terrain
#define TXT_OPTIONS_MENU                      145 // Options Menu
#define TXT_STOP                              146 // Stop
#define TXT_PLAY                              147 // Play
#define TXT_SHUFFLE                           148 // Shuffle
#define TXT_REPEAT                            149 // Repeat
#define TXT_MUSIC_VOLUME                      150 // Music volume:
#define TXT_SOUND_VOLUME                      151 // Sound volume:
#define TXT_ON                                152 // On
#define TXT_OFF                               153 // Off
#define TXT_MULTIPLAYER_GAME                  154 // Multiplayer Game
#define TXT_NO_FILES                          155 // No files available
#define TXT_DELETE_SINGLE_FILE                156 // Do you want to delete this file?
#define TXT_DELETE_MULTIPLE_FILES             157 // Do you want to delete %d files?
#define TXT_RESET_MENU                        158 // Reset Values
#define TXT_CONFIRM_EXIT                      159 // Do you want to abort the mission?
#define TXT_MISSION_DESCRIPTION               160 // Mission Description
#define TXT_C1                                161 // Joe
#define TXT_C2                                162 // Barry
#define TXT_C3                                163 // Shelly
#define TXT_C4                                164 // Maria
#define TXT_C5                                165 // Karen
#define TXT_C6                                166 // Steve
#define TXT_C7                                167 // Phil
#define TXT_C8                                168 // Dwight
#define TXT_C9                                169 // Erik
#define TXT_EINSTEIN                          170 // Prof. Einstein
#define TXT_BIB                               171 // Road Bib
#define TXT_FASTER                            172 // Faster
#define TXT_SLOWER                            173 // Slower
#define TXT_AIR_STRIKE                        174 // Air Strike
#define TXT_STEEL_CRATE                       175 // Steel Crate
#define TXT_WOOD_CRATE                        176 // Wood Crate
#define TXT_WATER_CRATE                       177 // Water Crate
#define TXT_FLAG_SPOT                         178 // Flag Location
#define TXT_UNABLE_READ_SCENARIO              179 // Unable to read scenario!
#define TXT_ERROR_LOADING_GAME                180 // Error loading game!
#define TXT_OBSOLETE_SAVEGAME                 181 // Obsolete saved game.
#define TXT_MUSTENTER_DESCRIPTION             182 // You must enter a description!
#define TXT_ERROR_SAVING_GAME                 183 // Error saving game!
#define TXT_DELETE_FILE_QUERY                 184 // Delete this file?
#define TXT_EMPTY_SLOT                        185 // [EMPTY SLOT]
#define TXT_SELECT_MPLAYER_GAME               186 // Select Multiplayer Game
#define TXT_MODEM_SERIAL                      187 // Modem/Serial
#define TXT_NETWORK                           188 // Network
#define TXT_INIT_NET_ERROR                    189 // Unable to initialize network!
#define TXT_JOIN_NETWORK_GAME                 190 // Join Network Game
#define TXT_NEW                               191 // New
#define TXT_JOIN                              192 // Join
#define TXT_SEND_MESSAGE                      193 // Send Message
#define TXT_YOUR_NAME                         194 // Your Name:
#define TXT_SIDE_COLON                        195 // Your Side:
#define TXT_COLOR_COLON                       196 // Your Color:
#define TXT_GAMES                             197 // Games
#define TXT_PLAYERS                           198 // Players
#define TXT_SCENARIO_COLON                    199 // Scenario:
#define TXT_NOT_FOUND                         200 // >> NOT FOUND <<
#define TXT_START_CREDITS_COLON               201 // Starting Credits:
#define TXT_BASES_COLON                       202 // Bases:
#define TXT_TIBERIUM_COLON                    203 // Ore:
#define TXT_CRATES_COLON                      204 // Crates:
#define TXT_AI_PLAYERS_COLON                  205 // AI Players:
#define TXT_REQUEST_DENIED                    206 // Request denied.
#define TXT_UNABLE_PLAY_WAAUGH                207 // Unable to play; scenario not found.
#define TXT_NOTHING_TO_JOIN                   208 // Nothing to join!
#define TXT_NAME_ERROR                        209 // You must enter a name!
#define TXT_DUPENAMES_NOTALLOWED              210 // Duplicate names are not allowed.
#define TXT_YOURGAME_OUTDATED                 211 // Your game version is outdated.
#define TXT_DESTGAME_OUTDATED                 212 // Destination game version is outdated.
#define TXT_THATGUYS_GAME                     213 // %s's Game
#define TXT_THATGUYS_GAME_BRACKET             214 // [%s's Game]
#define TXT_NETGAME_SETUP                     215 // Network Game Setup
#define TXT_REJECT                            216 // Reject
#define TXT_CANT_REJECT_SELF                  217 // You can't reject yourself!
#define TXT_SELECT_PLAYER_REJECT              218 // You must select a player to reject.
#define TXT_BASES                             219 // Bases
#define TXT_CRATES                            220 // Crates
#define TXT_AI_PLAYERS                        221 // AI Players
#define TXT_SCENARIOS                         222 // Scenarios
#define TXT_CREDITS_COLON                     223 // Credits:
#define TXT_ONLY_ONE                          224 // Only one player?
#define TXT_OOPS                              225 // Oops!
#define TXT_TO                                226 // To %s:
#define TXT_TO_ALL                            227 // To All:
#define TXT_MESSAGE                           228 // Message:
#define TXT_CONNECTION_LOST                   229 // Connection to %s lost!
#define TXT_LEFT_GAME                         230 // %s has left the game.
#define TXT_PLAYER_DEFEATED                   231 // %s has been defeated!
#define TXT_WAITING_CONNECT                   232 // Waiting to Connect...
#define TXT_NULL_CONNERR_CHECK_CABLES         233 // Connection error! Check your cables. Attempting to Reconnect...
#define TXT_MODEM_CONNERR_REDIALING           234 // Connection error! Redialing...
#define TXT_MODEM_CONNERR_WAITING             235 // Connection error! Waiting for Call...
#define TXT_SELECT_SERIAL_GAME                236 // Select Serial Game
#define TXT_DIAL_MODEM                        237 // Dial Modem
#define TXT_ANSWER_MODEM                      238 // Answer Modem
#define TXT_NULL_MODEM                        239 // Null Modem
#define TXT_SETTINGS                          240 // Settings
#define TXT_PORT_COLON                        241 // Port:
#define TXT_IRQ_COLON                         242 // IRQ:
#define TXT_BAUD_COLON                        243 // Baud:
#define TXT_INIT_STRING                       244 // Init String:
#define TXT_CWAIT_STRING                      245 // Call Waiting String:
#define TXT_TONE_BUTTON                       246 // Tone Dialing
#define TXT_PULSE_BUTTON                      247 // Pulse Dialing
#define TXT_HOST_SERIAL_GAME                  248 // Host Serial Game
#define TXT_OPPONENT_COLON                    249 // Opponent:
#define TXT_USER_SIGNED_OFF                   250 // User signed off!
#define TXT_JOIN_SERIAL_GAME                  251 // Join Serial Game
#define TXT_PHONE_LIST                        252 // Phone List
#define TXT_ADD                               253 // Add
#define TXT_EDIT                              254 // Edit
#define TXT_DIAL                              255 // Dial
#define TXT_DEFAULT                           256 // Default
#define TXT_DEFAULT_SETTINGS                  257 // Default Settings
#define TXT_CUSTOM_SETTINGS                   258 // Custom Settings
#define TXT_PHONE_LISTING                     259 // Phone Listing
#define TXT_NAME_COLON                        260 // Name:
#define TXT_NUMBER_COLON                      261 // Number:
#define TXT_UNABLE_FIND_MODEM                 262 // Unable to find modem. Check power and cables.
#define TXT_NO_CARRIER                        263 // No carrier.
#define TXT_LINE_BUSY                         264 // Line busy.
#define TXT_NUMBER_INVALID                    265 // Number invalid.
#define TXT_SYSTEM_NOT_RESPONDING             266 // Other system not responding!
#define TXT_OUT_OF_SYNC                       267 // Games are out of sync!
#define TXT_PACKET_TOO_LATE                   268 // Packet received too late!
#define TXT_PLAYER_LEFT_GAME                  269 // Other player has left the game.
#define TXT_FROM                              270 // From %s:%s
#define TXT_SCORE_TIME                        271 // TIME:
#define TXT_SCORE_LEAD                        272 // LEADERSHIP:
#define TXT_SCORE_EFFI                        273 // ECONOMY:
#define TXT_SCORE_TOTA                        274 // TOTAL SCORE:
#define TXT_SCORE_CASU                        275 // CASUALTIES:
#define TXT_SCORE_NEUT                        276 // NEUTRAL:
#define TXT_SCORE_BUIL                        277 // BUILDINGS LOST
#define TXT_SCORE_BUIL1                       278 // BUILDINGS
#define TXT_SCORE_BUIL2                       279 // LOST:
#define TXT_SCORE_TOP                         280 // TOP SCORES
#define TXT_SCORE_ENDCRED                     281 // ENDING CREDITS:
#define TXT_SCORE_TIMEFORMAT1                 282 // %dh %dm
#define TXT_SCORE_TIMEFORMAT2                 283 // %dm
#define TXT_DIALING                           284 // Dialing...
#define TXT_DIALING_CANCELED                  285 // Dialing Canceled
#define TXT_WAITING_FOR_CALL                  286 // Waiting for Call...
#define TXT_ANSWERING_CANCELED                287 // Answering Canceled
#define TXT_E6                                288 // Engineer
#define TXT_E8                                289 // Spy
#define TXT_MODEM_OR_LOOPBACK                 290 // Not a Null Modem Cable Attached! It is a modem or loopback cable.
#define TXT_MAP                               291 // Map
#define TXT_BLOSSOM_TREE                      292 // Blossom Tree
#define TXT_RESTATE_MISSION                   293 // Briefing
#define TXT_COMPUTER                          294 // Computer
#define TXT_COUNT                             295 // Unit Count:
#define TXT_LEVEL                             296 // Tech Level:
#define TXT_OPPONENT                          297 // Opponent
#define TXT_KILLS_COLON                       298 // Kills:
#define TXT_VIDEO                             299 // Video
#define TXT_C10                               300 // Scientist
#define TXT_CAPTURE_THE_FLAG                  301 // Capture The Flag
#define TXT_OBJECTIVE                         302 // Mission Objective
#define TXT_MISSION                           303 // Mission
#define TXT_NO_SAVES                          304 // No saved games available.
#define TXT_CIVILIAN_BUILDING                 305 // Civilian Building
#define TXT_TECHNICIAN                        306 // Technician
#define TXT_NO_SAVELOAD                       307 // Save game options are not allowed during a multiplayer session.
#define TXT_DELPHI                            308 // Special 1
#define TXT_TO_REPLAY                         309 // Would you like to replay this mission?
#define TXT_RECONN_TO                         310 // Reconnecting to %s.
#define TXT_PLEASE_WAIT                       311 // Please wait %02d seconds.
#define TXT_SURRENDER                         312 // Do you wish to surrender?
#define TXT_SEL_TRANS                         313 // SELECT TRANSMISSION
#define TXT_GAMENAME_MUSTBE_UNIQUE            314 // Your game name must be unique.
#define TXT_GAME_IS_CLOSED                    315 // Game is closed.
#define TXT_NAME_MUSTBE_UNIQUE                316 // Your name must be unique.
#define TXT_RECONNECTING_TO                   317 // Reconnecting to %s
#define TXT_WAITING_FOR_CONNECTIONS           318 // Waiting for connections...
#define TXT_TIME_ALLOWED                      319 // Time allowed: %02d seconds
#define TXT_PRESS_ESC                         320 // Press ESC to cancel.
#define TXT_JUST_YOU_AND_ME                   321 // From Computer: It's just you and me now!
#define TXT_CAPTURE_THE_FLAG_COLON            322 // Capture the Flag:
#define TXT_CHAN                              323 // Special 2
#define TXT_HAS_ALLIED                        324 // %s has allied with %s
#define TXT_AT_WAR                            325 // %s declares war on %s
#define TXT_SEL_TARGET                        326 // Select a target
#define TXT_RESIGN                            327 // Resign Game
#define TXT_TIBERIUM_FAST                     328 // Ore grows quickly.
#define TXT_ANSWERING                         329 // Answering...
#define TXT_INITIALIZING_MODEM                330 // Initializing Modem...
#define TXT_SCENARIOS_DO_NOT_MATCH            331 // Scenarios don't match.
#define TXT_POWER_OUTPUT                      332 // Power Output
#define TXT_POWER_OUTPUT_LOW                  333 // Power Output (low)
#define TXT_CONTINUE                          334 // Continue
#define TXT_QUEUE_FULL                        335 // Data Queue Overflow
#define TXT_SPECIAL_WARNING                   336 // %s changed game options!
#define TXT_CD_DIALOG_1                       337 // Please insert a Red Alert CD into the CD-ROM drive.
#define TXT_CD_DIALOG_2                       338 // Please insert CD %d (%s) into the CD-ROM drive.
#define TXT_CD_ERROR1                         339 // Red Alert is unable to detect your CD ROM drive.
#define TXT_NO_SOUND_CARD                     340 // No Sound Card Detected
#define TXT_UNKNOWN                           341 // UNKNOWN
#define TXT_OLD_GAME                          342 // (old)
#define TXT_NO_SPACE                          343 // Insufficient Disk Space to run Red Alert.
#define TXT_MUST_HAVE_SPACE                   344 // You must have %d megabytes of free disk space.
#define TXT_RUN_SETUP                         345 // Run SETUP program first.
#define TXT_WAITING_FOR_OPPONENT              346 // Waiting for Opponent
#define TXT_SELECT_SETTINGS                   347 // Please select 'Settings' to setup default configuration
#define TXT_PRISON                            348 // Prison
#define TXT_GAME_WAS_SAVED                    349 // Mission Saved
#define TXT_SPACE_CANT_SAVE                   350 // Insufficient disk space to save a game.
#define TXT_INVALID_PORT_ADDRESS              351 // Invalid Port/Address. COM 1-4 OR ADDRESS
#define TXT_INVALID_SETTINGS                  352 // Invalid Port and/or IRQ settings
#define TXT_IRQ_ALREADY_IN_USE                353 // IRQ already in use
#define TXT_ABORT                             354 // Abort
#define TXT_RESTART                           355 // Restart
#define TXT_RESTARTING                        356 // Mission is restarting. Please wait...
#define TXT_LOADING                           357 // Mission is loading. Please wait...
#define TXT_ERROR_IN_INITSTRING               358 // Error in the InitString
#define TXT_SHADOW_COLON                      359 // Shroud:
#define TXT_AVMINE                            360 // Anti-Vehicle Mine
#define TXT_APMINE                            361 // Anti-Personnel Mine
#define TXT_NEW_MISSIONS                      362 // New Missions
#define TXT_THIEF                             363 // Thief
#define TXT_MRJ                               364 // Radar Jammer
#define TXT_GAP_GENERATOR                     365 // Gap Generator
#define TXT_PILLBOX                           366 // Pillbox
#define TXT_CAMOPILLBOX                       367 // Camo. Pillbox
#define TXT_CHRONOSPHERE                      368 // Chronosphere
#define TXT_ENGLAND                           369 // England
#define TXT_GERMANY                           370 // Germany
#define TXT_SPAIN                             371 // Spain
#define TXT_USSR                              372 // Russia
#define TXT_UKRAINE                           373 // Ukraine
#define TXT_GREECE                            374 // Greece
#define TXT_FRANCE                            375 // France
#define TXT_TURKEY                            376 // Turkey
#define TXT_SHORE                             377 // Shore
#define TXT_PLACE_OBJECT                      378 // Select Object
#define TXT_SS                                379 // Submarine
#define TXT_DD                                380 // Destroyer
#define TXT_CA                                381 // Cruiser
#define TXT_TRANSPORT                         382 // Transport
#define TXT_PT                                383 // Gun Boat
#define TXT_LOBBY                             384 // Lobby
#define TXT_CHANNEL_GAMES                     385 // Games
#define TXT_SAVING_GAME                       386 // Save Game...
#define TXT_GAME_FULL                         387 // Game is full.
#define TXT_MUST_SELECT_GAME                  388 // You must select a game!
#define TXT_S_PLAYING_S                       389 // %s playing %s
#define TXT_ONLY_HOST_CAN_MODIFY              390 // Only the host can modify this option.
#define TXT_GAME_CANCELLED                    391 // Game was cancelled.
#define TXT_S_FORMED_NEW_GAME                 392 // %s has formed a new game.
#define TXT_GAME_NOW_IN_PROGRESS              393 // %s's game is now in progress.
#define TXT_TESLA                             394 // Tesla Coil
#define TXT_MGG                               395 // Mobile Gap Generator
#define TXT_FLAME_TURRET                      396 // Flame Tower
#define TXT_AAGUN                             397 // AA Gun
#define TXT_KENNEL                            398 // Kennel
#define TXT_SOVIET_TECH                       399 // Soviet Tech Center
#define TXT_BADGER                            400 // Badger Bomber
#define TXT_MIG                               401 // Mig Attack Plane
#define TXT_YAK                               402 // Yak Attack Plane
#define TXT_FENCE                             403 // Barbed Wire
#define TXT_MEDIC                             404 // Field Medic
#define TXT_SABOTEUR                          405 // Saboteur
#define TXT_GENERAL                           406 // General
#define TXT_E7                                407 // Tanya
#define TXT_PARA_BOMB                         408 // Parabombs
#define TXT_PARA_INFANTRY                     409 // Paratroopers
#define TXT_PARA_SABOTEUR                     410 // Parachute Saboteur
#define TXT_SHIP_YARD                         411 // Naval Yard
#define TXT_SUB_PEN                           412 // Sub Pen
#define TXT_SCENARIO_OPTIONS                  413 // Scenario Options
#define TXT_SPY_MISSION                       414 // Spy Plane
#define TXT_U2                                415 // Spy Plane
#define TXT_GUARD_DOG                         416 // Attack Dog
#define TXT_SPY_INFO                          417 // Spy Info
#define TXT_BUILDNGS                          418 // Buildings
#define TXT_UNITS                             419 // Units
#define TXT_INFANTRY                          420 // Infantry
#define TXT_AIRCRAFT                          421 // Aircraft
#define TXT_TRUCK                             422 // Supply Truck
#define TXT_INVUL                             423 // Invulnerability Device
#define TXT_IRON_CURTAIN                      424 // Iron Curtain
#define TXT_ADVANCED_TECH                     425 // Allied Tech Center
#define TXT_V2_LAUNCHER                       426 // V2 Rocket
#define TXT_FORWARD_COM                       427 // Forward Command Post
#define TXT_DEMOLITIONER                      428 // Demolitioner
#define TXT_MINE_LAYER                        429 // Mine Layer
#define TXT_FAKE_CONST                        430 // Fake Construction Yard
#define TXT_FAKE_WEAP                         431 // Fake War Factory
#define TXT_FAKE_YARD                         432 // Fake Naval Yard
#define TXT_FAKE_PEN                          433 // Fake Sub Pen
#define TXT_FAKE_RADAR                        434 // Fake Radar Dome
#define TXT_THEME_BIGF                        435 // Bigfoot
#define TXT_THEME_CRUS                        436 // Crush
#define TXT_THEME_FAC1                        437 // Face the Enemy 1
#define TXT_THEME_FAC2                        438 // Face the Enemy 2
#define TXT_THEME_HELL                        439 // Hell March
#define TXT_THEME_RUN1                        440 // Run for Your Life
#define TXT_THEME_SMSH                        441 // Smash
#define TXT_THEME_TREN                        442 // Trenches
#define TXT_THEME_WORK                        443 // Workmen
#define TXT_THEME_AWAIT                       444 // Await
#define TXT_THEME_DENSE_R                     445 // Dense
#define TXT_THEME_MAP                         446 // Map Selection
#define TXT_THEME_FOGGER1A                    447 // Fogger
#define TXT_THEME_MUD1A                       448 // Mud
#define TXT_THEME_RADIO2                      449 // Radio 2
#define TXT_THEME_ROLLOUT                     450 // Roll Out
#define TXT_THEME_SNAKE                       451 // Snake
#define TXT_THEME_TERMINAT                    452 // Terminate
#define TXT_THEME_TWIN                        453 // Twin
#define TXT_THEME_VECTOR1A                    454 // Vector
#define TXT_TEAM_MEMBERS                      455 // Team Members
#define TXT_BRIDGE                            456 // Bridge
#define TXT_BARREL                            457 // Barrel
#define TXT_GOODGUY                           458 // Friendly
#define TXT_BADGUY                            459 // Enemy
#define TXT_GOLD                              460 // Gold
#define TXT_GEMS                              461 // Gems
#define TXT_TEASER                            462 // Title Movie
#define TXT_MOVIES                            463 // Movies
#define TXT_INTERIOR                          464 // Interior
#define TXT_SONAR_PULSE                       465 // Sonar Pulse
#define TXT_MSLO                              466 // Missile Silo
#define TXT_GPS_SATELLITE                     467 // GPS Satellite
#define TXT_NUCLEAR_BOMB                      468 // Atom Bomb
#define TXT_EASY                              469 // Easy
#define TXT_HARD                              470 // Hard
#define TXT_NORMAL                            471 // Normal
#define TXT_DIFFICULTY                        472 // Please set the difficulty level. It will be used throughout this campaign.
#define TXT_ALLIES                            473 // Allies
#define TXT_SOVIET                            474 // Soviet
#define TXT_THEME_INTRO                       475 // Intro Theme
#define TXT_SHADOW_REGROWS                    476 // Shroud Regrows
#define TXT_ORE_SPREADS                       477 // Ore Regenerates
#define TXT_THEME_SCORE                       478 // Score Theme
#define TXT_INTERNET                          479 // Internet Game
#define TXT_ICE                               480 // Ice
#define TXT_CRATE                             481 // Crates
#define TXT_SKIRMISH                          482 // Skirmish
#define TXT_CHOOSE                            483 // Choose your side.
#define TXT_MINERALS                          484 // Valuable Minerals
#define TXT_IGNORE                            485 // Ignore
#define TXT_ERROR_NO_RESP                     486 // Error - modem is not responding.
#define TXT_ERROR_NO_RESCODE                  487 // Error - modem did not respond to result code enable command.
#define TXT_ERROR_NO_INIT                     488 // Error - modem did not respond to initialisation string.
#define TXT_ERROR_NO_VERB                     489 // Error - modem did not respond to 'verbose' command.
#define TXT_ERROR_NO_ECHO                     490 // Error - modem did not respond to 'echo' command.
#define TXT_ERROR_NO_DISABLE                  491 // Error - unable to disable modem auto answer.
#define TXT_ERROR_TOO_MANY                    492 // Error - Too many errors initialising modem - Aborting.
#define TXT_ERROR_ERROR                       493 // Error - Modem returned error status.
#define TXT_ERROR_TIMEOUT                     494 // Error - TIme out waiting for connect.
#define TXT_ACCOMPLISHED                      495 // Accomplished
#define TXT_CLICK_CONTINUE                    496 // Click to Continue
#define TXT_RECEIVING_SCENARIO                497 // Receiving scenario from host.
#define TXT_SENDING_SCENARIO                  498 // Sending scenario to remote players.
#define TXT_NO_FLOW_CONTROL_RESPONSE          499 // Error - Modem failed to respond to flow control command.
#define TXT_NO_COMPRESSION_RESPONSE           500 // Error - Modem failed to respond to compression command.
#define TXT_NO_ERROR_CORRECTION_RESPONSE      501 // Error - Modem failed to respond to error correction command.
#define TXT_EXPLAIN_REGISTRATION              502 // To play Red Alert via the internet you must be connected
#define TXT_ERROR_UNABLE_TO_RUN_WCHAT         503 // Wchat not installed. Please install it from either CD.
#define TXT_REGISTER                          504 // Register
#define TXT_ORE_MINE                          505 // Ore Mine
#define TXT_NO_REGISTERED_MODEM               506 // No registered modem
#define TXT_CHRONOSHIFT                       507 // Chronoshift
#define TXT_UNABLE_TO_OPEN_PORT               508 // Invalid Port or Port is in use
#define TXT_NO_DIAL_TONE                      509 // No dial tone. Ensure your modem is connected to the phone line and try again.
#define TXT_NO_EXPANSION_SCENARIO             510 // Error - other player does not have this expansion scenario.
#define TXT_STAND_BY                          511 // Please Stand By...
#define TXT_THEME_CREDITS                     512 // End Credits Theme
#define TXT_POWER_AAGUN                       513 // Low Power: AA-Guns offline
#define TXT_POWER_TESLA                       514 // Low Power: Tesla Coils offline
#define TXT_LOW_POWER                         515 // Low Power
#define TXT_COMMANDER                         516 // Commander:
#define TXT_BATTLES_WON                       517 // Battles Won:
#define TXT_MISMATCH                          518 // Game versions incompatible.
#define TXT_SCENARIO_ERROR                    519 // Incompatible scenario file detected. The scenario may be corrupt.
#define TXT_CONNECTING                        520 // Connecting...
#define TXT_MODEM_INITIALISATION              521 // Modem Initialization
#define TXT_DATA_COMPRESSION                  522 // Data Compression
#define TXT_ERROR_CORRECTION                  523 // Error Correction
#define TXT_HARDWARE_FLOW_CONTROL             524 // Hardware Flow Control
#define TXT_ADVANCED                          525 // Advanced
#define TXT_THEME_2ND_HAND                    526 // 2nd_Hand
#define TXT_THEME_ARAZOID                     527 // Arazoid
#define TXT_THEME_BACKSTAB                    528 // BackStab
#define TXT_THEME_CHAOS2                      529 // Chaos2
#define TXT_THEME_SHUT_IT                     530 // Shut_It
#define TXT_THEME_TWINMIX1                    531 // TwinMix1
#define TXT_THEME_UNDER3                      532 // Under3
#define TXT_THEME_VR2                         533 // VR2
#define TXT_ASK_EMERGENCY_SAVE_NOT_RESPONDING 534 // The other system is not responding.
#define TXT_ASK_EMERGENCY_SAVE_HUNG_UP        535 // The other system hung up.
#define TXT_NO_REG_APP                        536 // Red Alert was unable to run the registration software.
#define TXT_NO_CS_SCENARIOS                   537 // A player in the game does not have this expansion scenario.
#define TXT_MISSILESUB                        538 // Missile Sub
#define TXT_SHOCKTROOPER                      539 // Shock Trooper
#define TXT_MECHANIC                          540 // Mechanic
#define TXT_CHRONOTANK                        541 // Chrono Tank
#define TXT_TESLATANK                         542 // Tesla Tank
#define TXT_MAD                               543 // M.A.D. Tank
#define TXT_DEMOTRUCK                         544 // Demolition Truck
#define TXT_PHASETRANSPORT                    545 // Phase Transport
#define TXT_THEME_BOG                         546 // Bog
#define TXT_THEME_FLOAT_V2                    547 // Floating
#define TXT_THEME_GLOOM                       548 // Gloom
#define TXT_THEME_GRNDWIRE                    549 // Ground Wire
#define TXT_THEME_RPT                         550 // Mech Man 2
#define TXT_THEME_SEARCH                      551 // Search
#define TXT_THEME_TRACTION                    552 // Traction
#define TXT_THEME_WASTELND                    553 // Wasteland
#define TXT_CARRIER                           554 // Helicarrier
#define TXT_INSUFFICIENT_FUNDS                555 // Remasters addition, not in actual string table.
