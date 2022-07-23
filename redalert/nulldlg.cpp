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

/* $Header: /counterstrike/NULLDLG.CPP 14    3/17/97 1:05a Steve_tall $ */
/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D    S T U D I O S        **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Command & Conquer                        *
 *                                                                         *
 *                    File Name : NULLDLG.CPP                              *
 *                                                                         *
 *                   Programmer : Bill R. Randolph                         *
 *                                                                         *
 *                   Start Date : 04/29/95                                 *
 *                                                                         *
 *                  Last Update : Jan. 21, 1997 [V.Grippi]                 *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   Com_Scenario_Dialog -- Serial game scenario selection dialog		   *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "function.h"
#include "msgbox.h"
#include "drop.h"
#include "cheklist.h"
#include "colrlist.h"
#include "framelimit.h"
#include "statbtn.h"
#include "textbtn.h"

// PG
GameType Select_Serial_Dialog(void)
{
    return GAME_NORMAL;
}

int Reconnect_Modem(void)
{
    return 0;
}

#ifdef FIXIT_RANDOM_GAME
#include <time.h>
#endif

#ifdef FIXIT_CSII //	checked - ajw 9/28/98
extern bool Is_Mission_126x126(char* file_name);
extern bool Is_Mission_Counterstrike(char* file_name);
extern bool Is_Mission_Aftermath(char* file_name);
#endif

#ifdef ENGLISH
char const* EngMisStr[] = {"Coastal Influence (Med)",
                           "Middle Mayhem (Sm)",
                           "Equal Opportunity (Sm)",
                           "Marooned II (Med)",
                           "Keep off the Grass (Sm)",
                           "Isle of Fury (Lg)",
                           "Ivory Wastelands (Sm)",
                           "Shallow Grave (Med)",
                           "North By Northwest (Lg)",
                           "First Come, First Serve (Sm)",
                           "Island Hoppers (Sm)",
                           "Raraku (Lg)",
                           "Central Conflict (Lg)",
                           "Combat Alley (Med)",
                           "Island Wars (Lg)",
                           "Desolation (Lg)",
                           "No Escape (Med)",
                           "No Man's Land (Med)",
                           "Normandy (Med)",
                           "Pond Skirmish (Med)",
                           "Ridge War (Med)",
                           "A Path Beyond (Lg)",
                           "Dugout Isle (Med)",
                           "Treasure Isle (Med)",

                           "Africa (Lg)",
                           "Alaska Anarchy (Lg)",
                           "All that Glitters... (Lg)",
                           "Apre's Peace (Lg)",
                           "Antartica (Lg)",
                           "Armourgarden (Lg)",
                           "Austraila (Med)",
                           "Barrier to Entry (Lg)",
                           "Bavarian Blast (Med)",
                           "Be Shore (Med)",
                           "Bearing Straits (Med)",
                           "Blow Holes (Lg)",
                           "Bonsai (Sm)",
                           "Brother Stalin (Lg)",
                           "Bullseye (Lg)",
                           "C&C (Med)",
                           "Camos Canyon (Med)",
                           "Camos Coves (Lg)",
                           "Camos Cross (Lg)",
                           "Camos Crossing (Sm)",
                           "Central Arena (Lg)",
                           "Canyon River (Med)",
                           "Crossroads (Sm)",
                           "Czech Mate (Lg)",
                           "Dday (Med)",
                           "Disaster Central (Lg)",
                           "Docklands (Med)",
                           "East Coast (Med)",
                           "Eastern Seaboard (Lg)",
                           "Finger Lake (Lg)",
                           "Fjords (Med)",
                           "Floodlands (Lg)",
                           "Forest under fire (Lg)",
                           "Four Corners (Lg)",
                           "Frostbit Fjords (Lg)",
                           "Glenboig (Sm)",
                           "Hell Frozen Over (Lg)",
                           "India (Lg)",
                           "Indirect Fire (Lg)",
                           "Island Wars II (Lg)",
                           "Italy (Lg)",
                           "Kabalo (Lg)",
                           "King of the Hills (Lg)",
                           "Lake Divide (Med)",
                           "Lakelands (Lg)",
                           "Land Ladder (Lg)",
                           "Lotsa Lakes (Lg)",
                           "Lunar Battlefield (Lg Special)",
                           "Malibu Fields (Med)",
                           "Marshland (Med)",
                           "MyLai Delta (Med)",
                           "Natural Harbor (Med)",
                           "No Way Out (Lg)",
                           "Normandy Landing (Lg)",
                           "Ore Wars (Med)",
                           "Oz (Lg)",
                           "Pilgrim Fathers II (Lg)",
                           "Pip's Ice Tea (Med)",
                           "Polar Panic (Lg)",
                           "Ponds (Med)",
                           "Putney (Lg)",
                           "Return to Zion (Lg)",
                           "Ring of Land (Lg)",
                           "River Basin (Lg)",
                           "River Delta (Med)",
                           "River Islands (Med)",
                           "River Maze (Sm)",
                           "Rivers (Sm)",
                           "Run the Gauntlet (Med)",
                           "Scappa Flow (Lg)",
                           "Siberian Slaughter (Lg)",
                           "Sleepy Valley (Sm)",
                           "Snake River (Lg)",
                           "Snow Wars (Lg)",
                           "Snowball fight (Lg)",
                           "Snowy Island (Lg)",
                           "So Near So Far (Sm)",
                           "South America (Lg)",
                           "Spring Line (Lg)",
                           "Star (Lg)",
                           "Straighter & Narrower (Sm)",
                           "TerrainSpotting (Sm)",
                           "The Bay (Lg)",
                           "The Garden (Lg)",
                           "The Great Lakes (Med)",
                           "The Ice Arena (Lg)",
                           "The Lake District (Lg)",
                           "The Linked lands (Lg)",
                           "The Mississippi (Med)",
                           "The Sticky Bit (Lg)",
                           "The Valley (Med)",
                           "The Woods Today (Lg)",
                           "Things to Come (Lg)",
                           "Tiger Core (Sm)",
                           "To the Core (Sm)",
                           "Tournament Hills (Lg)",
                           "Tropical Storm (Med)",
                           "Tundra Trouble (Lg)",
                           "Uk (Med)",
                           "Undiscovered Country (Sm)",
                           "United States (Med)",
                           "Volcano (Sm)",
                           "Wastelands (Lg)",
                           "Water Works (Sm)",
                           "World Map (Med)",
                           "Zambezi (Lg)",

                           "A Pattern of Islands (Lg 8 players)",
                           "Arena Valley Extreme (Mega 8 players)",
                           "Around the Rim (Sm 4 players)",
                           "Ashes to Ashes (Lg 6 players)",
                           "Artic Wasteland (Mega 8 players)",
                           "Badajoz (Med 4 players)",
                           "Baptism of Fire (Lg 6 players)",
                           "Big Fish, Small Pond (Lg 6 players)",
                           "Blue Lakes (Lg 8 players)",
                           "Booby Traps (Mega 8 players)",
                           "Bridgehead (Lg 6 players)",
                           "Butterfly Bay (Lg 6 players)",
                           "Central Conflict Extreme (Mega 8 players)",
                           "Circles of Death (Mega 8 players)",
                           "Cold Front (Med 6 players)",
                           "Cold Pass (Med 4 players)",
                           "Combat Zones (Mega 8 players)",
                           "Conflict Cove (Sm 4 players)",
                           "Culloden Moor (Med 8 players)",
                           "Damnation Alley (Mega 8 players)",
                           "Death Valley (Mega 8 players)",
                           "Deep Six (Mega 8 players)",
                           "Destruction Derby (Mega 8 players)",
                           "Diamonds Aren't Forever (Mega 8 players)",
                           "Elysium (Sm 4 players)",
                           "Equal Shares (Lg 4 players)",
                           "Frost Bitten (Mega 8 players)",
                           "Frozen Valley (Med 6 players)",
                           "Gettysburg (Sm 4 players)",
                           "Glacial Valley (Sm 4 players)",
                           "Gold Coast (Med 6 players)",
                           "Gold Rush (Lg 4 players)",
                           "Habitat (Lg 4 players)",
                           "Hades Frozen Over (Sm 4 players)",
                           "Hamburger Hill (Mega 8 players)",
                           "Hastings (Sm 4 players)",
                           "Hell's Pass (Med 6 players)",
                           "Holy Grounds (Mega 8 players)",
                           "Ice Bergs (Med 6 players)",
                           "Ice Station (Lg 6 players)",
                           "Ice Queen (Lg 4 players)",
                           "In the Sun (Med 6 players)",
                           "Innocents? (Mega 8 players)",
                           "Islands (Med 8 players)",
                           "Island Plateau (Lg 4 players)",
                           "Island Wars Extreme (Mega 8 players)",
                           "Kananga (Med 6 players)",
                           "King of the Hills Extreme (Mega 8 players)",
                           "Lake Land (Lg 8 players)",
                           "Land Locked (Lg 8 players)",
                           "Lanes (Med 8 players)",
                           "Leipzip (Sm 4 players)",
                           "Meander (Lg 8 players)",
                           "Mekong (Med 8 players)",
                           "Middle Ground (Med 8 players)",
                           "Naval Conquests (Mega 8 players)",
                           "On your Marks (Med 4 players)",
                           "Open Warfare (Mega 8 players)",
                           "Ore Gardens (Lg 8 players)",
                           "Potholes (Mega 8 players)",
                           "Puddles (Med 4 players)",
                           "Random Violence (Mega 8 players)",
                           "Revenge (Med 8 players)",
                           "Rias (Med 8 players)",
                           "River Crossing (Sm 4 players)",
                           "River Rampage (Mega 8 players)",
                           "River Rapids (Lg 6 players)",
                           "Rivers Wild (Mega 8 players)",
                           "Rorkes Drift (Lg 4 players)",
                           "Seaside (Med 4 players)",
                           "Shades (Med 8 players)",
                           "Smuggler's Cove (Lg 6 players)",
                           "Snow Garden (Sm 2 players)",
                           "Stalingrad (Sm 4 players)",
                           "Sticks & Stones (Med 4 players)",
                           "Strathearn Valley (Lg 6 players)",
                           "Super Bridgehead (Mega 8 players)",
                           "Super Mekong (Mega 8 players)",
                           "Super Ore Gardens (Mega 8 players)",
                           "Switch (Med 4 players)",
                           "The Berg (Mega 8 players)",
                           "The Boyne (Med 4 players)",
                           "The Bulge (Sm 4 players)",
                           "The Cauldron (Lg 6 players)",
                           "The Finger (Lg 6 players)",
                           "The Hills Have Eyes (Mega 8 players)",
                           "The Keyes (Med 6 players)",
                           "The Lakes (Med 8 players)",
                           "The Neck (Med 6 players)",
                           "The Web (Lg 6 players)",
                           "To the Core (Lg 4 players)",
                           "Trafalgar (Lg 4 players)",
                           "Twin Rivers (Sm 4 players)",
                           "Umtumbo Gorge (Lg 4 players)",
                           "Watch Your Step Extreme (Mega 8 players)",
                           "Waterfalls (Lg 8 players)",
                           "Waterloo Revisited (Lg 6 players)",
                           "Water Werks (Mega 8 players)",
                           "Warlord's Lake (Sm 4 players)",
                           "Zama (Sm 4 players)",

                           NULL};
#endif

#ifdef GERMAN
char const* EngMisStr[] = {

    "A Path Beyond (Lg)",
    "Weg ins Jenseits (Gr)",
    "Central Conflict (Lg)",
    "Der zentrale Konflikt (Gr)",
    "Coastal Influence (Med)",
    "Sturm an der KÅste (Mit)",
    "Combat Alley (Med)",
    "Boulevard der Schlachten (Mit)",
    "Desolation (Lg)",
    "VerwÅstung (Gr)",
    "Dugout Isle (Med)",
    "Buddelschiff (Mit)",
    "Equal Opportunity (Sm)",
    "Gleiche Chancen (Kl)",
    "First Come, First Serve (Sm)",
    "Wer zuerst kommt... (Kl)",
    "Island Hoppers (Sm)",
    "Inselspringen (Kl)",
    "Island Wars (Lg)",
    "Der Krieg der Eilande (Gr)",
    "Isle of Fury (Lg)",
    "Insel des Zorns (Gr)",
    "Ivory Wastelands (Sm)",
    "ElfenbeinwÅste (Kl)",
    "Keep off the Grass (Sm)",
    "Rasen betreten verboten (Kl)",
    "Marooned II (Med)",
    "Gestrandet (Mit)",
    "Middle Mayhem (Sm)",
    "Mittelsmann (Kl)",
    "No Escape (Med)",
    "Kein Entrinnen (Mit)",
    "No Man's Land (Med)",
    "Niemandsland (Mit)",
    "Normandy (Med)",
    "Normandie (Mit)",
    "North By Northwest (Lg)",
    "Nord auf Nordwest (Gr)",
    "Pond Skirmish (Med)",
    "TeichgeplÑnkel (Mit)",
    "Raraku (Lg)",
    "Raraku (Gr)",
    "Ridge War (Med)",
    "Das Tal der Cyborgs (Mit)",
    "Shallow Grave (Med)",
    "Ein enges Grab (Mit)",
    "Treasure Isle (Med)",
    "Die Schatzinsel (Mit)",

    "Africa (Lg)",
    "Afrika (Gr)",
    "Alaska Anarchy (Lg)",
    "Anarchie in Alaska (Gr)",
    "All that Glitters... (Lg)",
    "Alles was glÑnzt... (Gr)",
    "Apre's Peace (Lg)",
    "Apres Frieden (Gr)",
    "Antartica (Lg)",
    "Antarktica (Gr)",
    "Armourgarden (Lg)",
    "Garten der Panzer (Gr)",
    "Austraila (Med)",
    "Koalaland (Mit)",
    "Barrier to Entry (Lg)",
    "Zutritt verboten (Gr)",
    "Bavarian Blast (Med)",
    "Bayrische Blasmusik (Mit)",
    "Be Shore (Med)",
    "StrandlÑufer (Mit)",
    "Bearing Straits (Med)",
    "Die Heringstrasse (Mit)",
    "Blow Holes (Lg)",
    "Lîcheriger KÑse (Gr)",
    "Bonsai (Sm)",
    "Bonsai (Kl)",
    "Brother Stalin (Lg)",
    "BrÅderchen Stalin (Gr)",
    "Bullseye (Lg)",
    "Bullseye (Gr)",
    "C&C (Med)",
    "C&C (Mit)",
    "Camos Canyon (Med)",
    "Camos-Canyon (Mit)",
    "Camos Coves (Lg)",
    "Camos-Grotte (Gr)",
    "Camos Cross (Lg)",
    "Camos-Kreuz (Gr)",
    "Camos Crossing (Sm)",
    "Camos-Kreuzweg (Kl)",
    "Central Arena (Lg)",
    "Spielplatz des Teufels (Gr)",
    "Canyon River (Med)",
    "Canyonfluss (Mit)",
    "Crossroads (Sm)",
    "Kreuzung (Kl)",
    "Czech Mate (Lg)",
    "Tschechische Erîffnung (Gr)",
    "Dday (Med)",
    "D-Day (Mit)",
    "Disaster Central (Lg)",
    "Endstation Schweinebucht (Gr)",
    "Docklands (Med)",
    "Docklands (Mit)",
    "East Coast (Med)",
    "OstkÅste (Mit)",
    "Eastern Seaboard (Lg)",
    "Die Passage nach Osten (Gr)",
    "Finger Lake (Lg)",
    "Fingersee (Gr)",
    "Fjords (Med)",
    "Fjorde (Mit)",
    "Floodlands (Lg)",
    "Land unter! (Gr)",
    "Forest under fire (Lg)",
    "Waldsterben im Feuer (Gr)",
    "Four Corners (Lg)",
    "Viereck (Gr)",
    "Frostbit Fjords (Lg)",
    "Frostbeulenfjord (Gr)",
    "Glenboig (Sm)",
    "Glenboig (Kl)",
    "Hell Frozen Over (Lg)",
    "Winter in der Hîlle (Gr)",
    "India (Lg)",
    "Indien (Gr)",
    "Indirect Fire (Lg)",
    "Indirekter Beschuss (Gr)",
    "Island Wars II (Lg)",
    "Krieg der Inseln (Gr)",
    "Italy (Lg)",
    "Italien (Gr)",
    "Kabalo (Lg)",
    "Kabalo (Gr)",
    "King of the Hills (Lg)",
    "Kînig des MaulwurfshÅgels (Gr)",
    "Lake Divide (Med)",
    "Wasserscheide (Mit)",
    "Lakelands (Lg)",
    "Seenplatte (Gr)",
    "Land Ladder (Lg)",
    "Das Leiterspiel (Gr)",
    "Lotsa Lakes (Lg)",
    "Mehr Seen (Gr)",
    "Lunar Battlefield (Lg Special)",
    "Schlachtfeld Mond (Gr Spezial)",
    "Malibu Fields (Med)",
    "Malibu (Mit)",
    "Marshland (Med)",
    "Schlammschlacht (Mit)",
    "MyLai Delta (Med)",
    "Das Delta von My Lai (Mit)",
    "Natural Harbor (Med)",
    "NatÅrlicher Hafen (Mit)",
    "No Way Out (Lg)",
    "Kein Entkommen (Gr)",
    "Normandy Landing (Lg)",
    "Landung in der Normandie (Gr)",
    "Ore Wars (Med)",
    "Die Erz-Kriege (Mit)",
    "Oz (Lg)",
    "Das Land Oz (Gr)",
    "Pilgrim Fathers II (Lg)",
    "Die GrÅndervÑter (Gr)",
    "Pip's Ice Tea (Med)",
    "Pips Eistee (Mit)",
    "Polar Panic (Lg)",
    "Panik am Pol (Gr)",
    "Ponds (Med)",
    "TÅmpelspringer (Mit)",
    "Putney (Lg)",
    "Putney (Gr)",
    "Return to Zion (Lg)",
    "RÅckkehr nach Zion (Gr)",
    "Ring of Land (Lg)",
    "Der Landring (Gr)",
    "River Basin (Lg)",
    "Flusslauf (Gr)",
    "River Delta (Med)",
    "Flussdelta (Mit)",
    "River Islands (Med)",
    "Flussinsel (Mit)",
    "River Maze (Sm)",
    "Flussgewirr (Kl)",
    "Rivers (Sm)",
    "FlÅsse (Kl)",
    "Run the Gauntlet (Med)",
    "Spiessrutenlauf (Mit)",
    "Scappa Flow (Lg)",
    "Scapa Flow (Gr)",
    "Siberian Slaughter (Lg)",
    "Sibirisches Gemetzel (Gr)",
    "Sleepy Valley (Sm)",
    "Tal der Ahnungslosen (Kl)",
    "Snake River (Lg)",
    "Am Schlangenfluss (Gr)",
    "Snow Wars (Lg)",
    "Krieg der Flocken (Gr)",
    "Snowball fight (Lg)",
    "Schneeballschlacht (Gr)",
    "Snowy Island (Lg)",
    "Schneeinsel (Gr)",
    "So Near So Far (Sm)",
    "So nah und doch so fern (Kl)",
    "South America (Lg)",
    "SÅdamerika (Gr)",
    "Spring Line (Lg)",
    "FrÅhlingsgefÅhle (Gr)",
    "Star (Lg)",
    "Stern (Gr)",
    "Straighter & Narrower (Sm)",
    "Enger & schmaler (Kl)",
    "TerrainSpotting (Sm)",
    "TerrainSpotting (Kl)",
    "The Bay (Lg)",
    "Die Bucht (Gr)",
    "The Garden (Lg)",
    "Der Garten (Gr)",
    "The Great Lakes (Med)",
    "Die Grossen Seen (Mit)",
    "The Ice Arena (Lg)",
    "Eisarena (Gr)",
    "The Lake District (Lg)",
    "Kalte Seenplatte (Gr)",
    "The Linked lands (Lg)",
    "Die verbundenen LÑnder (Gr)",
    "The Mississippi (Med)",
    "GrÅsse von Tom Sawyer (Mit)",
    "The Sticky Bit (Lg)",
    "Der klebrige Teil (Gr)",
    "The Valley (Med)",
    "Das Tal (Mit)",
    "The Woods Today (Lg)",
    "WaldlÑufer (Gr)",
    "Things to Come (Lg)",
    "Was die Zukunft bringt (Gr)",
    "Tiger Core (Sm)",
    "Das Herz des Tigers (Kl)",
    "To the Core (Sm)",
    "Mitten ins Herz (Kl)",
    "Tournament Hills (Lg)",
    "HÅgel der Entscheidung (Gr)",
    "Tropical Storm (Med)",
    "TropenstÅrme (Mit)",
    "Tundra Trouble (Lg)",
    "Tauziehen in der Tundra (Gr)",
    "Uk (Med)",
    "GB (Mit)",
    "Undiscovered Country (Sm)",
    "Unentdecktes Land (Kl)",
    "United States (Med)",
    "US (Mit)",
    "Volcano (Sm)",
    "Vulkan (Kl)",
    "Wastelands (Lg)",
    "WÅstenei (Gr)",
    "Water Works (Sm)",
    "Wasserwerke (Kl)",
    "World Map (Med)",
    "Weltkarte (Kl)",
    "Zambezi (Lg)",
    "Sambesi (Gr)",

    //#if 0
    "A Pattern of Islands (Lg 8 players)",
    "Inselmuster (gross, 8 Spieler)",
    "Arena Valley Extreme (Mega 8 players)",
    "Arenatal (sehr gross, 8 Spieler)",
    "Around the Rim (Sm 4 players)",
    "Um die Kante (klein, 4 Spieler)",
    "Ashes to Ashes (Lg 6 players)",
    "Asche zu Asche (gross, 6 Spieler)",
    "Artic Wasteland (Mega 8 players)",
    "Arktische WÅste (sehr gross, 8 Spieler)",
    "Badajoz (Med 4 players)",
    "Badjoz (mittelgross, 4 Spieler)",
    "Baptism of Fire (Lg 6 players)",
    "Feuertaufe (gross, 6 Spieler)",
    "Big Fish, Small Pond (Lg 6 players)",
    "Grosser Fisch im kleinen Teich (gross, 6 Spieler)",
    "Blue Lakes (Lg 8 players)",
    "Die blauen Seen (gross, 8 Spieler)",
    "Booby Traps (Mega 8 players)",
    "Vorsicht, Falle! (sehr gross, 8 Spieler)",
    "Bridgehead (Lg 6 players)",
    "BrÅckenkopf im Niemandsland (gross, 6 Spieler)",
    "Butterfly Bay (Lg 6 players)",
    "Schmetterlingsbucht (gross, 6 Spieler)",
    "Central Conflict Extreme (Mega 8 players)",
    "Zentraler Konflikt fÅr Kînner (sehr gross, 8 Spieler)",
    "Circles of Death (Mega 8 players)",
    "Todeskreise (sehr gross, 8 Spieler)",
    "Cold Front (Med 6 players)",
    "Kaltfront ( mittelgross, 6 Spieler)",
    "Cold Pass (Med 4 players)",
    "Cooler Pass (mittelgross, 4 Spieler)",
    "Combat Zones (Mega 8 players)",
    "Kampfgebiete (sehr gross, 8 Spieler)",
    "Conflict Cove (Sm 4 players)",
    "Hîhlenkonflikt (klein, 4 Spieler)",
    "Culloden Moor (Med 8 players)",
    "Culloden-Moor (mittelgross, 8 Spieler)",
    "Damnation Alley (Mega 8 players)",
    "Strasse der Verdammten (sehr gross, 8 Spieler)",
    "Death Valley (Mega 8 players)",
    "Tal des Todes (sehr gross, 8 Spieler)",
    "Deep Six (Mega 8 players)",
    "Tiefe Sechs (sehr gross, 8 Spieler)",
    "Destruction Derby (Mega 8 players)",
    "Destruction Derby (sehr gross, 8 Spieler)",
    "Diamonds Aren't Forever (Mega 8 players)",
    "VergÑngliche Diamanten (sehr gross, 8 Spieler)",
    "Elysium (Sm 4 players)",
    "Elysium (klein, 4 Spieler)",
    "Equal Shares (Lg 4 players)",
    "Gleiche Anteile (gross, 4 Spieler)",
    "Frost Bitten (Mega 8 players)",
    "Frostbrand (sehr gross, 8 Spieler)",
    "Frozen Valley (Med 6 players)",
    "Eisiges Tal (mittelgross, 6 Spieler)",
    "Gettysburg (Sm 4 players)",
    "Gettysburg (klein, 4 Spieler)",
    "Glacial Valley (Sm 4 players)",
    "Gletschertal (klein, 4 Spieler)",
    "Gold Coast (Med 6 players)",
    "GoldkÅste (mittelgross, 6 Spieler)",
    "Gold Rush (Lg 4 players)",
    "Goldrausch (gross, 4 Spieler)",
    "Habitat (Lg 4 players)",
    "Habitat (gross, 4 Spieler)",
    "Hades Frozen Over (Sm 4 players)",
    "Frostschutz fÅr die Hîlle (klein, 4 Spieler)",
    "Hamburger Hill (Mega 8 players)",
    "Hamburger Hill (sehr gross, 8 Spieler)",
    "Hastings (Sm 4 players)",
    "Hastings (klein, 4 Spieler)",
    "Hell's Pass (Med 6 players)",
    "Hîllenpass (mittelgross, 6 Spieler)",
    "Holy Grounds (Mega 8 players)",
    "Heiliger Boden (sehr gross, 8 Spieler)",
    "Ice Bergs (Med 6 players)",
    "Eisberge (mittelgross, 6 Spieler)",
    "Ice Station (Lg 6 players)",
    "Eisstation  (gross, 6 Spieler)",
    "Ice Queen (Lg 4 players)",
    "Eiskînigin (gross, 4 Spieler)",
    "In the Sun (Med 6 players)",
    "Unter der Sonne (mittelgross, 6 Spieler)",
    "Innocents? (Mega 8 players)",
    "Unschuldig? Wer? (sehr gross, 8 Spieler)",
    "Islands (Med 8 players)",
    "Inseln im Nebel (mittelgross, 8 Spieler)",
    "Island Plateau (Lg 4 players)",
    "Inselplateau (gross, 4 Spieler)",
    "Island Wars Extreme (Mega 8 players)",
    "Extremes Inselspringen (sehr gross, 8 Spieler)",
    "Kananga (Med 6 players)",
    "Kananga (mittelgross, 6 Spieler)",
    "King of the Hills Extreme (Mega 8 players)",
    "Kînig des MaulwurfshÅgels (sehr gross, 8 Spieler)",
    "Lake Land (Lg 8 players)",
    "Seenland (gross, 8 Spieler)",
    "Land Locked (Lg 8 players)",
    "Das Verschlossene Land (gross, 8 Spieler)",
    "Lanes (Med 8 players)",
    "Gassenjungen (mittelgross, 8 Spieler)",
    "Leipzip (Sm 4 players)",
    "Leipzig (klein, 4 Spieler)",
    "Meander (Lg 8 players)",
    "MÑander (gross, 8 Spieler)",
    "Mekong (Med 8 players)",
    "Mekong (mittelgross, 8 Spieler)",
    "Middle Ground (Med 8 players)",
    "Mittelsmann (mittelgross, 8 Spieler)",
    "Naval Conquests (Mega 8 players)",
    "Kommt zur Marine, haben sie gesagt (sehr gross, 8 Spieler)",
    "On your Marks (Med 4 players)",
    "Auf die PlÑtze (mittelgross, 4 Spieler)",
    "Open Warfare (Mega 8 players)",
    "Offener Schlagabtausch (sehr gross, 8 Spieler)",
    "Ore Gardens (Lg 8 players)",
    "Erzparadies (gross, 8 Spieler)",
    "Potholes (Mega 8 players)",
    "Schlaglîcher (sehr gross, 8 Spieler)",
    "Puddles (Med 4 players)",
    "PfÅtzen (mittelgross, 4 Spieler)",
    "Random Violence (Mega 8 players)",
    "Unberechenbare Gewalt (sehr gross, 8 Spieler)",
    "Revenge (Med 8 players)",
    "Rache (mittelgross, 8 Spieler)",
    "Rias (Med 8 players)",
    "Kabul (mittelgross, 8 Spieler)",
    "River Crossing (Sm 4 players)",
    "Die Furt (klein, 4 Spieler)",
    "River Rampage (Mega 8 players)",
    "Flussfahrt (sehr gross, 8 Spieler)",
    "River Rapids (Lg 6 players)",
    "Stromschnellen (gross, 6 Spieler)",
    "Rivers Wild (Mega 8 players)",
    "Wildwasser (sehr gross, 8 Spieler)",
    "Rorkes Drift (Lg 4 players)",
    "Rorkes Drift (gross, 4 Spieler)",
    "Seaside (Med 4 players)",
    "Strandleben (mittelgross, 4 Spieler)",
    "Shades (Med 8 players)",
    "Schattenreich (mittelgross, 8 Spieler)",
    "Smuggler's Cove (Lg 6 players)",
    "Schmugglerhîhle (gross, 6 Spieler)",
    "Snow Garden (Sm 2 players)",
    "Schneegestîber (klein, 2 Spieler)",
    "Stalingrad (Sm 4 players)",
    "Stalingrad (klein, 4 Spieler)",
    "Sticks & Stones (Med 4 players)",
    "Holz und Steine (mittelgross, 4 Spieler)",
    "Strathearn Valley (Lg 6 players)",
    "Das Tal von Strathearn (gross, 6 Spieler)",
    "Super Bridgehead (Mega 8 players)",
    "Super-BrÅckenkopf (sehr gross, 8 Spieler)",
    "Super Mekong (Mega 8 players)",
    "Super-Mekong (sehr gross, 8 Spieler)",
    "Super Ore Gardens (Mega 8 players)",
    "Super-Erzparadies (sehr gross, 8 Spieler)",
    "Switch (Med 4 players)",
    "Schalter (mittelgross, 4 Spieler)",
    "The Berg (Mega 8 players)",
    "Der Berg (sehr gross, 8 Spieler)",
    "The Boyne (Med 4 players)",
    "Boyne (mittelgross, 4 Spieler)",
    "The Bulge (Sm 4 players)",
    "Die Wîlbung (klein, 4 Spieler)",
    "The Cauldron (Lg 6 players)",
    "Der Kessel (gross, 6 Spieler)",
    "The Finger (Lg 6 players)",
    "Der Finger (gross, 6 Spieler)",
    "The Hills Have Eyes (Mega 8 players)",
    "Die HÅgel haben Augen (sehr gross, 8 Spieler)",
    "The Keyes (Med 6 players)",
    "Ein Sumpf (mittelgross, 6 Spieler)",
    "The Lakes (Med 8 players)",
    "Die Seen (mittelgross, 8 Spieler)",
    "The Neck (Med 6 players)",
    "Der Hals (mittelgross, 6 Spieler)",
    "The Web (Lg 6 players)",
    "Das Netz (gross, 6 Spieler)",
    "To the Core (Lg 4 players)",
    "Mitten ins Herz (gross, 4 Spieler)",
    "Trafalgar (Lg 4 players)",
    "Trafalgar (gross, 4 Spieler)",
    "Twin Rivers (Sm 4 players)",
    "Zwillingsstrîme (klein, 4 Spieler)",
    "Umtumbo Gorge (Lg 4 players)",
    "Die Umtumbo-Schlucht (gross, 4 Spieler)",
    "Watch Your Step Extreme (Mega 8 players)",
    "Vorsicht, Lebensgefahr (sehr gross, 8 Spieler)",
    "Waterfalls (Lg 8 players)",
    "Wasserfall (gross, 8 Spieler)",
    "Waterloo Revisited (Lg 6 players)",
    "Zu Besuch in Waterloo (gross, 6 Spieler)",
    "Water Werks (Mega 8 players)",
    "Wasserwerk (sehr gross, 8 Spieler)",
    "Warlord's Lake (Sm 4 players)",
    "Der See des Kriegsgottes (klein, 4 Spieler)",
    "Zama (Sm 4 players)",
    "Zama (klein, 4 Spieler)",
    //#endif
    NULL};
#endif
#ifdef FRENCH
char const* EngMisStr[] = {

    "A Path Beyond (Lg)",
    "Le Passage (Max)",
    "Central Conflict (Lg)",
    "Conflit Central (Max)",
    "Coastal Influence (Med)",
    "Le Chant des Canons (Moy)",
    "Combat Alley (Med)",
    "Aux Armes! (Moy)",
    "Desolation (Lg)",
    "DÇsolation (Max)",
    "Dugout Isle (Med)",
    "L'Ile Maudite (Moy)",
    "Equal Opportunity (Sm)",
    "A Chances Egales (Min)",
    "First Come, First Serve (Sm)",
    "La Loi du Plus Fort (Min)",
    "Island Hoppers (Sm)",
    "D'une Ile Ö l'autre (Min)",
    "Island Wars (Lg)",
    "Guerres Insulaires (Max)",
    "Isle of Fury (Lg)",
    "L'Ile de la Furie(Max)",
    "Ivory Wastelands (Sm)",
    "Terres d'Ivoire (Min)",
    "Keep off the Grass (Sm)",
    "Hors de mon Chemin (Min)",
    "Marooned II (Med)",
    "Isolement II (Moy)",
    "Middle Mayhem (Sm)",
    "Chaos Interne (Min)",
    "No Escape (Med)",
    "Le Piäge (Moy)",
    "No Man's Land (Med)",
    "No Man's Land (Moy)",
    "Normandy (Med)",
    "Normandie (Moy)",
    "North By Northwest (Lg)",
    "Nord, Nord-Ouest (Max)",
    "Pond Skirmish (Med)",
    "Bain de Sang (Moy)",
    "Raraku (Lg)",
    "Raraku (Max)",
    "Ridge War (Med)",
    "Guerre au Sommet (Moy)",
    "Shallow Grave (Med)",
    "La Saveur de la Mort (Moy)",
    "Treasure Isle (Med)",
    "L'Ile au TrÇsor (Moy)",

    "Africa (Lg)",
    "Afrique (Max)",
    "Alaska Anarchy (Lg)",
    "Anarchie en Alaska (Max)",
    "All that Glitters... (Lg)",
    "Tout ce qui brille... (Max)",
    "Apre's Peace (Lg)",
    "Une Paix Durement NÇgociÇe... (Max)",
    "Antartica (Lg)",
    "Antarctique (Max)",
    "Armourgarden (Lg)",
    "La Guerre des BlindÇs (Max)",
    "Austraila (Med)",
    "Australie (Moy)",
    "Barrier to Entry (Lg)",
    "Barriäre Ö l'EntrÇe (Max)",
    "Bavarian Blast (Med)",
    "Tonnerre Bavarois (Moy)",
    "Be Shore (Med)",
    "Plages MenacÇes (Moy)",
    "Bearing Straits (Med)",
    "Droit Devant ! (Moy)",
    "Blow Holes (Lg)",
    "Cratäres (Max)",
    "Bonsai (Sm)",
    "Bonsaã (Min)",
    "Brother Stalin (Lg)",
    "Fräre Staline (Max)",
    "Bullseye (Lg)",
    "L'oeil du Taureau (Max)",
    "C&C (Med)",
    "C&C (Moy)",
    "Camos Canyon (Med)",
    "Le Canyon (Moy)",
    "Camos Coves (Lg)",
    "Criques (Max)",
    "Camos Cross (Lg)",
    "La Croix de Guerre (Max)",
    "Camos Crossing (Sm)",
    "La CroisÇe des Chemins (Min)",
    "Central Arena (Lg)",
    "L'Aräne Diabolique (Max)",
    "Canyon River (Med)",
    "Au Milieu Coule Une Riviäre (Moy)",
    "Crossroads (Sm)",
    "Carrefours (Min)",
    "Czech Mate (Lg)",
    "Tchäque et Mat (Max)",
    "Dday (Med)",
    "Le Jour J (Moy)",
    "Disaster Central (Lg)",
    "DÇsastre Central (Max)",
    "Docklands (Med)",
    "L'Enfer des Docks (Moy)",
    "East Coast (Med)",
    "Cìte Est (Moy)",
    "Eastern Seaboard (Lg)",
    "Rivages de l'Est (Max)",
    "Finger Lake (Lg)",
    "Le Lac de tous les Dangers (Max)",
    "Fjords (Med)",
    "Fjords (Moy)",
    "Floodlands (Lg)",
    "Campagne Lacustre (Max)",
    "Forest under fire (Lg)",
    "Foràt en flammes (Max)",
    "Four Corners (Lg)",
    "4 Coins (Max)",
    "Frostbit Fjords (Lg)",
    "Fjords GelÇs (Max)",
    "Glenboig (Sm)",
    "Glenboig (Min)",
    "Hell Frozen Over (Lg)",
    "Enfer de Glace Max)",
    "India (Lg)",
    "Inde (Max)",
    "Indirect Fire (Lg)",
    "Attaque Indirecte (Max)",
    "Island Wars II (Lg)",
    "Guerres Insulaires II (Max)",
    "Italy (Lg)",
    "Italie (Max)",
    "Kabalo (Lg)",
    "Kabalo (Max)",
    "King of the Hills (Lg)",
    "Le Roi des Montagnes (Max)",
    "Lake Divide (Med)",
    "La Guerre du Lac (Moy)",
    "Lakelands (Lg)",
    "Terres SubmergÇes (Max)",
    "Land Ladder (Lg)",
    "Jusqu'au Sommet (Max)",
    "Lotsa Lakes (Lg)",
    "Terres de Lacs (Max)",
    "Lunar Battlefield (Lg Special)",
    "Combat Lunaire (Max SpÇcial)",
    "Malibu Fields (Med)",
    "Les Champs de Malibu (Moy)",
    "Marshland (Med)",
    "MarÇcages (Moy)",
    "MyLai Delta (Med)",
    "Le Delta Mylai (Moy)",
    "Natural Harbor (Med)",
    "Port Naturel (Moy)",
    "No Way Out (Lg)",
    "Sans Issue (Max)",
    "Normandy Landing (Lg)",
    "Le DÇbarquement (Max)",
    "Ore Wars (Med)",
    "La Guerre du Minerai (Moy)",
    "Oz (Lg)",
    "Oz (Max)",
    "Pilgrim Fathers II (Lg)",
    "Les Pälerins 2 (Max)",
    "Pip's Ice Tea (Med)",
    "Les TranchÇes de Glace (Moy)",
    "Polar Panic (Lg)",
    "Panique Polaire (Max)",
    "Ponds (Med)",
    "Les Etangs (Moy)",
    "Putney (Lg)",
    "La Meilleure DÇfense... (Max)",
    "Return to Zion (Lg)",
    "Retour Ö Sion (Max)",
    "Ring of Land (Lg)",
    "Le Cycle Infernal (Max)",
    "River Basin (Lg)",
    "Confrontation Navale (Max)",
    "River Delta (Med)",
    "Le Delta (Moy)",
    "River Islands (Med)",
    "Cìtes Ö Surveiller de Präs (Moy)",
    "River Maze (Sm)",
    "Labyrinthe Fluvial (Min)",
    "Rivers (Sm)",
    "Riviäres (Min)",
    "Run the Gauntlet (Med)",
    "Relevons le DÇfi ! (Moy)",
    "Scappa Flow (Lg)",
    "Combats Sanglants (Max)",
    "Siberian Slaughter (Lg)",
    "Carnage SibÇrien (Max)",
    "Sleepy Valley (Sm)",
    "La VallÇe Endormie (Min)",
    "Snake River (Lg)",
    "La Riviäre aux Serpents (Max)",
    "Snow Wars (Lg)",
    "Guerres de Neige (Max)",
    "Snowball fight (Lg)",
    "Bataille de Boules de Neige (Max)",
    "Snowy Island (Lg)",
    "L'Ile sous la Neige (Max)",
    "So Near So Far (Sm)",
    "Si Loin, Si Proche (Min)",
    "South America (Lg)",
    "AmÇrique du Sud (Max)",
    "Spring Line (Lg)",
    "Ligne de Front (Max)",
    "Star (Lg)",
    "Etoile (Max)",
    "Straighter & Narrower (Sm)",
    "L'Entonnoir (Min)",
    "TerrainSpotting (Sm)",
    "TerrainSpotting (Min)",
    "The Bay (Lg)",
    "La Baie (Max)",
    "The Garden (Lg)",
    "Le Jardin (Max)",
    "The Great Lakes (Med)",
    "Les Grands Lacs (Moy)",
    "The Ice Arena (Lg)",
    "L'Aräne de Glace (Max)",
    "The Lake District (Lg)",
    "Un Lac Imprenable (Max)",
    "The Linked lands (Lg)",
    "Passages Ö GuÇ (Max)",
    "The Mississippi (Med)",
    "Mississippi (Moy)",
    "The Sticky Bit (Lg)",
    "Marasme (Max)",
    "The Valley (Med)",
    "La VallÇe (Moy)",
    "The Woods Today (Lg)",
    "Aujoud'hui: la Mort ! (Max)",
    "Things to Come (Lg)",
    "DÇnouement Incertain (Max)",
    "Tiger Core (Sm)",
    "Le Coeur du Tigre (Min)",
    "To the Core (Sm)",
    "Le Coeur du Conflit (Min)",
    "Tournament Hills (Lg)",
    "Combat en Altitude (Max)",
    "Tropical Storm (Med)",
    "Ouragan Tropical (Moy)",
    "Tundra Trouble (Lg)",
    "La Toundra (Max)",
    "Uk (Med)",
    "Royaume Uni (Moy)",
    "Undiscovered Country (Sm)",
    "Terre Inconnue (Min)",
    "United States (Med)",
    "Etats Unis (Moy)",
    "Volcano (Sm)",
    "Le Volcan (Min)",
    "Wastelands (Lg)",
    "Terres DÇsolÇes (Max)",
    "Water Works (Sm)",
    "Jeux d'Eau (Min)",
    "World Map (Med)",
    "Carte du Monde (Moy)",
    "Zambezi (Lg)",
    "Zambäze (Max)",
    //#if 0
    "A Pattern of Islands (Lg 8 players)",
    "Archipel (Max. 8 joueurs)",
    "Arena Valley Extreme (Mega 8 players)",
    "La VallÇe de l'aräne (XL 8 joueurs)",
    "Around the Rim (Sm 4 players)",
    "Autour de la cràte (Min. 4 joueurs)",
    "Ashes to Ashes (Lg 6 players)",
    "RÇduit en cendres (Max. 6 joueurs)",
    "Artic Wasteland (Mega 8 players)",
    "DÇsolation arctique (XL 8 joueurs)",
    "Badajoz (Med 4 players)",
    "Badjoz (Moy. 4 joueurs)",
    "Baptism of Fire (Lg 6 players)",
    "Baptàme du feu (Max. 6 joueurs)",
    "Big Fish, Small Pond (Lg 6 players)",
    "Gros poisson, Min. Mare (Max. 6 joueurs)",
    "Blue Lakes (Lg 8 players)",
    "Lacs bleus (Max. 8 joueurs)",
    "Booby Traps (Mega 8 players)",
    "Piäges (XL 8 joueurs)",
    "Bridgehead (Lg 6 players)",
    "Tàte de pont (Max. 6 joueurs)",
    "Butterfly Bay (Lg 6 players)",
    "La baie du papillon (Max. 6 joueurs)",
    "Central Conflict Extreme (Mega 8 players)",
    "Conflit central extràme (XL 8 joueurs)",
    "Circles of Death (Mega 8 players)",
    "Les cercles de la mort (XL 8 joueurs)",
    "Cold Front (Med 6 players)",
    "Front froid ( Moy. 6 joueurs)",
    "Cold Pass (Med 4 players)",
    "La Passe GlacÇe (Moy. 4 joueurs)",
    "Combat Zones (Mega 8 players)",
    "Zones de combat (XL 8 joueurs)",
    "Conflict Cove (Sm 4 players)",
    "La Crique du conflit (Min. 4 joueurs)",
    "Culloden Moor (Med 8 players)",
    "La Lande de Culloden (Moy. 8 joueurs)",
    "Damnation Alley (Mega 8 players)",
    "Le chemin de la damnation (XL 8 joueurs)",
    "Death Valley (Mega 8 players)",
    "La vallÇe de la mort (XL 8 joueurs)",
    "Deep Six (Mega 8 players)",
    "Six de profondeur (XL 8 joueurs)",
    "Destruction Derby (Mega 8 players)",
    "Stock car (XL 8 joueurs)",
    "Diamonds Aren't Forever (Mega 8 players)",
    "Les diamants ne sont pas Çternels (XL 8 joueurs)",
    "Elysium (Sm 4 players)",
    "ElysÇe (Min. 4 joueurs)",
    "Equal Shares (Lg 4 players)",
    "Parts Çgales (Max. 4 joueurs)",
    "Frost Bitten (Mega 8 players)",
    "Engelures (XL 8 joueurs)",
    "Frozen Valley (Med 6 players)",
    "La VallÇe glacÇe (Moy. 6 joueurs)",
    "Gettysburg (Sm 4 players)",
    "Gettysburg (Min. 4 joueurs)",
    "Glacial Valley (Sm 4 players)",
    "VallÇe de glace (Min. 4 joueurs)",
    "Gold Coast (Med 6 players)",
    "La cìte dorÇe (Moy. 6 joueurs)",
    "Gold Rush (Lg 4 players)",
    "La ruÇe vers l'or (Max. 4 joueurs)",
    "Habitat (Lg 4 players)",
    "Habitat (Max. 4 joueurs)",
    "Hades Frozen Over (Sm 4 players)",
    "Les enfers glacÇs (Min. 4 joueurs)",
    "Hamburger Hill (Mega 8 players)",
    "Hamburger Hill (XL 8 joueurs)",
    "Hastings (Sm 4 players)",
    "Hastings (Min. 4 joueurs)",
    "Hell's Pass (Med 6 players)",
    "La route de l'enfer (Moy. 6 joueurs)",
    "Holy Grounds (Mega 8 players)",
    "Terres saintes (XL 8 joueurs)",
    "Ice Bergs (Med 6 players)",
    "Icebergs (Moy. 6 joueurs)",
    "Ice Station (Lg 6 players)",
    "Station glacÇe (Max. 6 joueurs)",
    "Ice Queen (Lg 4 players)",
    "Reine des glaces (Max. 4 joueurs)",
    "In the Sun (Med 6 players)",
    "Sous le soleil (Moy. 6 joueurs)",
    "Innocents? (Mega 8 players)",
    "Innocents ? (XL 8 joueurs)",
    "Islands (Med 8 players)",
    "Iles (Moy. 8 joueurs)",
    "Island Plateau (Lg 4 players)",
    "Plateau des åles (Max. 4 joueurs)",
    "Island Wars Extreme (Mega 8 players)",
    "Guerres insulaires extràme (XL 8 joueurs)",
    "Kananga (Med 6 players)",
    "Kananga (Moy. 6 joueurs)",
    "King of the Hills Extreme (Mega 8 players)",
    "Roi des collines extràme (XL 8 joueurs)",
    "Lake Land (Lg 8 players)",
    "Paysage lacustre (Max. 8 joueurs)",
    "Land Locked (Lg 8 players)",
    "Enclave (Max. 8 joueurs)",
    "Lanes (Med 8 players)",
    "Le parcours du combattant (Moy. 8 joueurs)",
    "Leipzip (Sm 4 players)",
    "Leipzig (Min. 4 joueurs)",
    "Meander (Lg 8 players)",
    "MÇandre (Max. 8 joueurs)",
    "Mekong (Med 8 players)",
    "MÇkong (Moy. 8 joueurs)",
    "Middle Ground (Med 8 players)",
    "Plateau mÇdian (Moy. 8 joueurs)",
    "Naval Conquests (Mega 8 players)",
    "Conquàtes navales (XL 8 joueurs)",
    "On your Marks (Med 4 players)",
    "A vos marques (Moy. 4 joueurs)",
    "Open Warfare (Mega 8 players)",
    "Guerre ouverte (XL 8 joueurs)",
    "Ore Gardens (Lg 8 players)",
    "Jardins de minerai (Max. 8 joueurs)",
    "Potholes (Mega 8 players)",
    "Nids de poules (XL 8 joueurs)",
    "Puddles (Med 4 players)",
    "Flaques (Moy. 4 joueurs)",
    "Random Violence (Mega 8 players)",
    "Violence alÇatoire (XL 8 joueurs)",
    "Revenge (Med 8 players)",
    "Vengeance (Moy. 8 joueurs)",
    "Rias (Med 8 players)",
    "Rias (Moy. 8 joueurs)",
    "River Crossing (Sm 4 players)",
    "Passage Ö guÇ (Min. 4 joueurs)",
    "River Rampage (Mega 8 players)",
    "Riviäre dÇchaånÇe (XL 8 joueurs)",
    "River Rapids (Lg 6 players)",
    "Rapides (Max. 6 joueurs)",
    "Rivers Wild (Mega 8 players)",
    "Riviäres sauvages (XL 8 joueurs)",
    "Rorkes Drift (Lg 4 players)",
    "L'Exode de Rorkes (Max. 4 joueurs)",
    "Seaside (Med 4 players)",
    "Cìte (Moy. 4 joueurs)",
    "Shades (Med 8 players)",
    "Ombres (Moy. 8 joueurs)",
    "Smuggler's Cove (Lg 6 players)",
    "La Crique du contrebandier (Max. 6 joueurs)",
    "Snow Garden (Sm 2 players)",
    "Jardin de neige (Min. 2 joueurs)",
    "Stalingrad (Sm 4 players)",
    "Stalingrad (Min. 4 joueurs)",
    "Sticks & Stones (Med 4 players)",
    "BÉton & Roches (Moy. 4 joueurs)",
    "Strathearn Valley (Lg 6 players)",
    "La VallÇe de Strathearn (Max. 6 joueurs)",
    "Super Bridgehead (Mega 8 players)",
    "Super tàte de pont (XL 8 joueurs)",
    "Super Mekong (Mega 8 players)",
    "Super MÇkong (XL 8 joueurs)",
    "Super Ore Gardens (Mega 8 players)",
    "Super jardin de minerai (XL 8 joueurs)",
    "Switch (Med 4 players)",
    "Permutation (Moy. 4 joueurs)",
    "The Berg (Mega 8 players)",
    "Le Berg (XL 8 joueurs)",
    "The Boyne (Med 4 players)",
    "Le Boyne (Moy. 4 joueurs)",
    "The Bulge (Sm 4 players)",
    "Le bombement (Min. 4 joueurs)",
    "The Cauldron (Lg 6 players)",
    "Le chaudron (Max. 6 joueurs)",
    "The Finger (Lg 6 players)",
    "Le doigt (Max. 6 joueurs)",
    "The Hills Have Eyes (Mega 8 players)",
    "Les collines ont des yeux (XL 8 joueurs)",
    "The Keyes (Med 6 players)",
    "Les Keyes (Moy. 6 joueurs)",
    "The Lakes (Med 8 players)",
    "Les lacs (Moy. 8 joueurs)",
    "The Neck (Med 6 players)",
    "Le goulot (Moy. 6 joueurs)",
    "The Web (Lg 6 players)",
    "La toile (Max. 6 joueurs)",
    "To the Core (Lg 4 players)",
    "Jusqu'au cour (Max. 4 joueurs)",
    "Trafalgar (Lg 4 players)",
    "Trafalgar (Max. 4 joueurs)",
    "Twin Rivers (Sm 4 players)",
    "Les deux riviäres (Min. 4 joueurs)",
    "Umtumbo Gorge (Lg 4 players)",
    "La Gorge de Umtumbo (Max. 4 joueurs)",
    "Watch Your Step Extreme (Mega 8 players)",
    "Pas-Ö-pas extràme (XL 8 joueurs)",
    "Waterfalls (Lg 8 players)",
    "Chutes d'eau (Max. 8 joueurs)",
    "Waterloo Revisited (Lg 6 players)",
    "Waterloo II (Max. 6 joueurs)",
    "Water Werks (Mega 8 players)",
    "Jeux d'eau (XL 8 joueurs)",
    "Warlord's Lake (Sm 4 players)",
    "Le lac du guerrier (Min. 4 joueurs)",
    "Zama (Sm 4 players)",
    "Zama (Min. 4 joueurs)",
    //#endif
    NULL};
#endif

#ifndef REMASTER_BUILD
/***********************************************************************************************
 * Find_Local_Scenario -- finds the file name of the scenario with matching attributes         *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    ptr to Scenario description                                                       *
 *           ptr to Scenario filename to fix up                                                *
 *           length of file for trivial rejection of scenario files                            *
 *           ptr to digest. Digests must match.                                                *
 *                                                                                             *
 *                                                                                             *
 * OUTPUT:   true if scenario is available locally                                             *
 *                                                                                             *
 * WARNINGS: We need to reject files that don't match exactly because scenarios with the same  *
 *           description can exist on both machines but have different contents. For example   *
 *           there will be lots of scenarios called 'my map' and 'aaaaaa'.          			  *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    8/23/96 12:36PM ST : Created                                                             *
 *=============================================================================================*/
bool Find_Local_Scenario(char* description, char* filename, unsigned int length, char* digest, bool official)
{
    char digest_buffer[32];
    /*
	** Scan through the scenario list looking for scenarios with matching descriptions.
	*/
    for (int index = 0; index < Session.Scenarios.Count(); index++) {
        if (!strcmp(Session.Scenarios[index]->Description(), description)) {
            CCFileClass file(Session.Scenarios[index]->Get_Filename());

            /*
			** Possible rejection on the basis of availability.
			*/
            if (file.Is_Available()) {
                /*
				** Possible rejection on the basis of size.
				*/
                if (file.Size() == length) {
                    /*
					** We don't know the digest for 'official' scenarios so assume its correct
					*/
                    if (!official) {
                        /*
						** Possible rejection on the basis of digest
						*/
                        INIClass ini;
                        ini.Load(file);
                        ini.Get_String(
                            "Digest", "1", "No digest here mate. Nope.", digest_buffer, sizeof(digest_buffer));
                    }
#ifdef FIXIT_CSII //	checked - ajw 9/28/98. But don't know why this happens. Because of autodownload?
                    /*
					** If this is an aftermath scenario then ignore the digest and return success.
					*/
                    if (Is_Mission_Aftermath((char*)Session.Scenarios[index]->Get_Filename())) {
                        strcpy(filename, Session.Scenarios[index]->Get_Filename());
                        return (true);
                    }
#endif

                    /*
					** This must be the same scenario. Copy the name and return true.
					*/
                    if (official || !strcmp(digest, digest_buffer)) {
                        strcpy(filename, Session.Scenarios[index]->Get_Filename());
                        return (true);
                    }
                }
            }
        }
    }

    /*
	** Couldnt find the scenario locally. Return failure.
	*/
    return (false);
}

/***********************************************************************************************
 * Com_Scenario_Dialog -- Serial game scenario selection dialog										  *
 *                                                                         						  *
 *                                                                         						  *
 * INPUT:                                                                  						  *
 *		none. *
 *                                                                         						  *
 * OUTPUT:                                                                 						  *
 *		true = success, false = cancel																			  *
 *                                                                         						  *
 * WARNINGS:                                                               						  *
 *		none. *
 *                                                                         						  *
 * HISTORY:                                                                						  *
 *   02/14/1995 BR : Created.
 *   01/21/97 V.Grippi added check for CS before sending scenario file *
 *=============================================================================================*/
int Com_Scenario_Dialog(bool skirmish)
{
    int factor = (SeenBuff.Get_Width() == 320) ? 1 : 2;
    /*........................................................................
    Dialog & button dimensions
    ........................................................................*/
    int d_dialog_w = 320 * factor;                      // dialog width
    int d_dialog_h = 200 * factor;                      // dialog height
    int d_dialog_x = ((320 * factor - d_dialog_w) / 2); // dialog x-coord
    int d_dialog_y = ((200 * factor - d_dialog_h) / 2); // dialog y-coord
    int d_dialog_cx = d_dialog_x + (d_dialog_w / 2);    // center x-coord

    int d_txt6_h = 6 * factor + 1; // ht of 6-pt text
    int d_margin1 = 5 * factor;    // margin width/height
    int d_margin2 = 7 * factor;    // margin width/height

    int d_name_w = 70 * factor;
    int d_name_h = 9 * factor;
    int d_name_x = d_dialog_x + (d_dialog_w / 4) - (d_name_w / 2);
    int d_name_y = d_dialog_y + d_margin2 + d_txt6_h + 1 * factor;

    int d_house_w = 60 * factor;
    int d_house_h = (8 * 5 * factor);
    int d_house_x = d_dialog_cx - (d_house_w / 2);
    int d_house_y = d_name_y;

    int d_color_w = 10 * factor;
    int d_color_h = 9 * factor;
    int d_color_y = d_name_y;
    int d_color_x = d_dialog_x + ((d_dialog_w / 4) * 3) - (d_color_w * 3);

    int d_playerlist_w = 118 * factor;
    int d_playerlist_h = (6 * 6 * factor) + 3 * factor; // 6 rows high
    int d_playerlist_x = d_dialog_x + d_margin1 + d_margin1 + 5 * factor;
    int d_playerlist_y = d_color_y + d_color_h + d_margin2 + 2 * factor /*KO + d_txt6_h*/;

    int d_scenariolist_w = 162 * factor;
    int d_scenariolist_h = (6 * 6 * factor) + 3 * factor; // 6 rows high
    d_scenariolist_h *= 2;

    int d_scenariolist_x = d_dialog_x + d_dialog_w - d_margin1 - d_margin1 - d_scenariolist_w - 5 * factor;
    int d_scenariolist_y = d_color_y + d_color_h + d_margin2 + 2 * factor;
    d_scenariolist_x = d_dialog_x + (d_dialog_w - d_scenariolist_w) / 2;

    int d_count_w = 25 * factor;
    int d_count_h = d_txt6_h;
    int d_count_x = d_playerlist_x + (d_playerlist_w / 2) + 20 * factor; // (fudged)
    int d_count_y = d_playerlist_y + d_playerlist_h + (d_margin1 * 2) - 2 * factor;
    d_count_y = d_scenariolist_y + d_scenariolist_h + d_margin1 - 2 * factor;

    int d_level_w = 25 * factor;
    int d_level_h = d_txt6_h;
    int d_level_x = d_playerlist_x + (d_playerlist_w / 2) + 20 * factor; // (fudged)
    int d_level_y = d_count_y + d_count_h;

    int d_credits_w = 25 * factor;
    int d_credits_h = d_txt6_h;
    int d_credits_x = d_playerlist_x + (d_playerlist_w / 2) + 20 * factor; // (fudged)
    int d_credits_y = d_level_y + d_level_h;

    int d_aiplayers_w = 25 * factor;
    int d_aiplayers_h = d_txt6_h;
    int d_aiplayers_x = d_playerlist_x + (d_playerlist_w / 2) + 20 * factor; // (fudged)
    int d_aiplayers_y = d_credits_y + d_credits_h;

    int d_options_w = 106 * factor;
    int d_options_h = (5 * 6 * factor) + 4 * factor;
    int d_options_x = d_dialog_x + d_dialog_w - 149 * factor;
    int d_options_y = d_scenariolist_y + d_scenariolist_h + d_margin1 - 2 * factor;

    int d_message_w = d_dialog_w - (d_margin1 * 2) - 20 * factor;
    int d_message_h = (8 * d_txt6_h) + 3 * factor; // 4 rows high
    int d_message_x = d_dialog_x + d_margin1 + 10 * factor;
    int d_message_y = d_options_y + d_options_h + 2 * factor;

    int d_send_w = d_dialog_w - (d_margin1 * 2) - 20 * factor;
    int d_send_h = 9 * factor;
    int d_send_x = d_dialog_x + d_margin1 + 10 * factor;
    int d_send_y = d_message_y + d_message_h;

    int d_ok_w = 45 * factor;
    int d_ok_h = 9 * factor;
    int d_ok_x = d_dialog_x + (d_dialog_w / 6) - (d_ok_w / 2);
    int d_ok_y = d_dialog_y + d_dialog_h - d_ok_h - d_margin1 - factor * 6;

    int d_cancel_w = 45 * factor;
    int d_cancel_h = 9 * factor;
    int d_cancel_x = d_dialog_cx - (d_cancel_w / 2);
    int d_cancel_y = d_dialog_y + d_dialog_h - d_cancel_h - d_margin1 - factor * 6;

    int d_load_w = 45 * factor;
    int d_load_h = 9 * factor;
    int d_load_x = d_dialog_x + ((d_dialog_w * 5) / 6) - (d_load_w / 2);
    int d_load_y = d_dialog_y + d_dialog_h - d_load_h - d_margin1 - factor * 6;

    /*........................................................................
    Button Enumerations
    ........................................................................*/
    enum
    {
        BUTTON_NAME = 100,
        BUTTON_HOUSE,
        BUTTON_CREDITS,
        BUTTON_AIPLAYERS,
        BUTTON_OPTIONS,
        BUTTON_PLAYERLIST,
        BUTTON_SCENARIOLIST,
        BUTTON_COUNT,
        BUTTON_LEVEL,
        BUTTON_OK,
        BUTTON_LOAD,
        BUTTON_CANCEL,
        BUTTON_DIFFICULTY,
    };

    /*........................................................................
    Redraw values: in order from "top" to "bottom" layer of the dialog
    ........................................................................*/
    typedef enum
    {
        REDRAW_NONE = 0,
        REDRAW_PARMS,
        REDRAW_MESSAGE,
        REDRAW_COLORS,
        REDRAW_BUTTONS,
        REDRAW_BACKGROUND,
        REDRAW_ALL = REDRAW_BACKGROUND
    } RedrawType;

    /*........................................................................
    Dialog variables
    ........................................................................*/
    RedrawType display = REDRAW_ALL; // redraw level
    bool process = true;             // process while true
    KeyNumType input;

    int playertabs[] = {77 * factor};     // tabs for player list box
    int optiontabs[] = {8};               // tabs for player list box
    char namebuf[MPLAYER_NAME_MAX] = {0}; // buffer for player's name
    bool transmit;                        // 1 = re-transmit new game options
    int cbox_x[] = {d_color_x,
                    d_color_x + d_color_w,
                    d_color_x + (d_color_w * 2),
                    d_color_x + (d_color_w * 3),
                    d_color_x + (d_color_w * 4),
                    d_color_x + (d_color_w * 5),
                    d_color_x + (d_color_w * 6),
                    d_color_x + (d_color_w * 7)};
    bool parms_received = false; // 1 = game options received
    bool changed = false;        // 1 = user has changed an option

    int rc;
    int recsignedoff = false;
    int i;
    unsigned int timingtime;
    unsigned int lastmsgtime;
    unsigned int lastredrawtime;
    unsigned int transmittime = 0;
    unsigned int theirresponsetime;
    static bool first_time = true;
    bool oppscorescreen = false;
    bool gameoptions = Session.Type == GAME_SKIRMISH;
    unsigned int msg_timeout = 1200; // init to 20 seconds

    CCFileClass loadfile("SAVEGAME.NET");
    bool load_game = false; // 1 = load a saved game
    NodeNameType* who;      // node to add to Players
    char* item;             // for filling in lists
    RemapControlType* scheme = GadgetClass::Get_Color_Scheme();
    bool messages_have_focus = true; // Gadget focus starts on the message system

    Set_Logic_Page(SeenBuff);

    CDTimerClass<SystemTimerClass> kludge_timer; // Timer to allow a wait after client joins
                                                 // game before game can start
    bool ok_button_added = false;

    /*........................................................................
    Buttons
    ........................................................................*/
    GadgetClass* commands; // button list

    EditClass name_edt(BUTTON_NAME,
                       namebuf,
                       MPLAYER_NAME_MAX,
                       TPF_TEXT,
                       d_name_x,
                       d_name_y,
                       d_name_w,
                       d_name_h,
                       EditClass::ALPHANUMERIC);

    char housetext[25] = "";
    Fancy_Text_Print("", 0, 0, 0, 0, TPF_TEXT);
    DropListClass housebtn(BUTTON_HOUSE,
                           housetext,
                           sizeof(housetext),
                           TPF_TEXT,
                           d_house_x,
                           d_house_y,
                           d_house_w,
                           d_house_h,
                           MFCD::Retrieve("BTN-UP.SHP"),
                           MFCD::Retrieve("BTN-DN.SHP"));
    ColorListClass playerlist(BUTTON_PLAYERLIST,
                              d_playerlist_x,
                              d_playerlist_y,
                              d_playerlist_w,
                              d_playerlist_h,
                              TPF_TEXT,
                              MFCD::Retrieve("BTN-UP.SHP"),
                              MFCD::Retrieve("BTN-DN.SHP"));
    ListClass scenariolist(BUTTON_SCENARIOLIST,
                           d_scenariolist_x,
                           d_scenariolist_y,
                           d_scenariolist_w,
                           d_scenariolist_h,
                           TPF_TEXT,
                           MFCD::Retrieve("BTN-UP.SHP"),
                           MFCD::Retrieve("BTN-DN.SHP"));
    GaugeClass countgauge(BUTTON_COUNT, d_count_x, d_count_y, d_count_w, d_count_h);

    char staticcountbuff[35];
    StaticButtonClass staticcount(0, "     ", TPF_TEXT, d_count_x + d_count_w + 3 * factor, d_count_y);

    GaugeClass levelgauge(BUTTON_LEVEL, d_level_x, d_level_y, d_level_w, d_level_h);

    char staticlevelbuff[35];
    StaticButtonClass staticlevel(0, "     ", TPF_TEXT, d_level_x + d_level_w + 3 * factor, d_level_y);

    GaugeClass creditsgauge(BUTTON_CREDITS, d_credits_x, d_credits_y, d_credits_w, d_credits_h);

    char staticcreditsbuff[35];
    StaticButtonClass staticcredits(0, "         ", TPF_TEXT, d_credits_x + d_credits_w + 3 * factor, d_credits_y);

    GaugeClass aiplayersgauge(BUTTON_AIPLAYERS, d_aiplayers_x, d_aiplayers_y, d_aiplayers_w, d_aiplayers_h);

    char staticaibuff[35];
    StaticButtonClass staticai(0, "     ", TPF_TEXT, d_aiplayers_x + d_aiplayers_w + 3 * factor, d_aiplayers_y);

    CheckListClass optionlist(BUTTON_OPTIONS,
                              d_options_x,
                              d_options_y,
                              d_options_w,
                              d_options_h,
                              TPF_TEXT,
                              MFCD::Retrieve("BTN-UP.SHP"),
                              MFCD::Retrieve("BTN-DN.SHP"));
    TextButtonClass okbtn(BUTTON_OK, TXT_OK, TPF_BUTTON, d_ok_x, d_ok_y, d_ok_w, d_ok_h);
    TextButtonClass loadbtn(BUTTON_LOAD, TXT_LOAD_BUTTON, TPF_BUTTON, d_load_x, d_load_y, d_load_w, d_load_h);
    TextButtonClass cancelbtn(BUTTON_CANCEL, TXT_CANCEL, TPF_BUTTON, d_cancel_x, d_cancel_y, d_cancel_w, d_cancel_h);

    SliderClass difficulty(BUTTON_DIFFICULTY,
                           d_name_x,
                           optionlist.Y + optionlist.Height + d_margin1 + d_margin1,
                           d_dialog_w - (d_name_x - d_dialog_x) * 2,
                           8 * factor,
                           true);
    if (Rule.IsFineDifficulty) {
        difficulty.Set_Maximum(5);
        difficulty.Set_Value(2);
    } else {
        difficulty.Set_Maximum(3);
        difficulty.Set_Value(1);
    }

    /*
    ------------------------- Build the button list --------------------------
    */
    commands = &name_edt;
    staticcount.Add_Tail(*commands);
    staticcredits.Add_Tail(*commands);
    staticai.Add_Tail(*commands);
    staticlevel.Add_Tail(*commands);
    difficulty.Add_Tail(*commands);
    scenariolist.Add_Tail(*commands);
    countgauge.Add_Tail(*commands);
    levelgauge.Add_Tail(*commands);
    creditsgauge.Add_Tail(*commands);
    aiplayersgauge.Add_Tail(*commands);
    optionlist.Add_Tail(*commands);
    if (Session.Type == GAME_SKIRMISH) {
        okbtn.Add_Tail(*commands);
    }
    cancelbtn.Add_Tail(*commands);
    cancelbtn.X = loadbtn.X;
    housebtn.Add_Tail(*commands);

    /*
    ----------------------------- Various Inits ------------------------------
    */
    /*........................................................................
    Init player name & house
    ........................................................................*/
    Session.ColorIdx = Session.PrefColor; // init my preferred color
    strcpy(namebuf, Session.Handle);      // set my name
    name_edt.Set_Text(namebuf, MPLAYER_NAME_MAX);
    name_edt.Set_Color(&ColorRemaps[(Session.ColorIdx == PCOLOR_DIALOG_BLUE) ? PCOLOR_REALLY_BLUE : Session.ColorIdx]);

    for (HousesType house = HOUSE_USSR; house <= HOUSE_FRANCE; house++) {
        housebtn.Add_Item(Text_String(HouseTypeClass::As_Reference(house).Full_Name()));
    }
    housebtn.Set_Selected_Index(Session.House - HOUSE_USSR);
    housebtn.Set_Read_Only(true);

    /*........................................................................
    Init scenario values, only the first time through
    ........................................................................*/
    Special.IsCaptureTheFlag = Rule.IsMPCaptureTheFlag;
    if (first_time) {
        Session.Options.Credits = Rule.MPDefaultMoney; // init credits & credit buffer
        Session.Options.Bases = Rule.IsMPBasesOn;      // init scenario parameters
        Session.Options.Tiberium = Rule.IsMPTiberiumGrow;
        Session.Options.Goodies = Rule.IsMPCrates;
        Session.Options.AIPlayers = 0;
        Special.IsShadowGrow = Rule.IsMPShadowGrow;
        Session.Options.UnitCount =
            (SessionClass::CountMax[Session.Options.Bases] + SessionClass::CountMin[Session.Options.Bases]) / 2;
        first_time = false;
    }

    /*........................................................................
    Init button states
    ........................................................................*/
    playerlist.Set_Tabs(playertabs);
    playerlist.Set_Selected_Style(ColorListClass::SELECT_NORMAL);

    optionlist.Set_Tabs(optiontabs);
    optionlist.Set_Read_Only(0);

    optionlist.Add_Item(Text_String(TXT_BASES));
    optionlist.Add_Item(Text_String(TXT_ORE_SPREADS));
    optionlist.Add_Item(Text_String(TXT_CRATES));
    optionlist.Add_Item(Text_String(TXT_SHADOW_REGROWS));
    optionlist.Add_Item(Text_String(TXT_CAPTURE_THE_FLAG));

    optionlist.Check_Item(0, Session.Options.Bases);
    optionlist.Check_Item(1, Session.Options.Tiberium);
    optionlist.Check_Item(2, Session.Options.Goodies);
    optionlist.Check_Item(3, Special.IsShadowGrow);
    optionlist.Check_Item(4, Special.IsCaptureTheFlag);

    countgauge.Set_Maximum(SessionClass::CountMax[Session.Options.Bases]
                           - SessionClass::CountMin[Session.Options.Bases]);
    countgauge.Set_Value(Session.Options.UnitCount - SessionClass::CountMin[Session.Options.Bases]);

    levelgauge.Set_Maximum(MPLAYER_BUILD_LEVEL_MAX - 1);
    levelgauge.Set_Value(BuildLevel - 1);

    creditsgauge.Set_Maximum(Rule.MPMaxMoney);
    creditsgauge.Set_Value(Session.Options.Credits);

    int maxp = Rule.MaxPlayers - 2;
    aiplayersgauge.Set_Maximum(maxp);

    if (Session.Options.AIPlayers > 7) {
        Session.Options.AIPlayers = 7;
    }
    Session.Options.AIPlayers = max(Session.Options.AIPlayers, 1);

    aiplayersgauge.Set_Value(Session.Options.AIPlayers - 1);

    /*........................................................................
    Init other scenario parameters
    ........................................................................*/
    Special.IsTGrowth =     // Session.Options.Tiberium;
        Rule.IsTGrowth =    // Session.Options.Tiberium;
        Special.IsTSpread = // Session.Options.Tiberium;
        Rule.IsTSpread = Session.Options.Tiberium;
    transmit = true;

    /*........................................................................
    Clear the Players vector
    ........................................................................*/
    Clear_Vector(&Session.Players);

    /*........................................................................
    Init scenario description list box
    ........................................................................*/
    for (i = 0; i < Session.Scenarios.Count(); i++) {
        int j;
        for (j = 0; EngMisStr[j] != NULL; j++) {
            if (!strcmp(Session.Scenarios[i]->Description(), EngMisStr[j])) {
#ifdef FIXIT_CSII //	ajw Added Aftermath installed checks (before, it was assumed).
                //	Add mission if it's available to us.
                if (!((Is_Mission_Counterstrike((char*)(Session.Scenarios[i]->Get_Filename()))
                       && !Is_Counterstrike_Installed())
                      || (Is_Mission_Aftermath((char*)(Session.Scenarios[i]->Get_Filename()))
                          && !Is_Aftermath_Installed())))
#endif
#if defined(GERMAN) || defined(FRENCH)
                    scenariolist.Add_Item(EngMisStr[j + 1]);
#else
                scenariolist.Add_Item(EngMisStr[j]);
#endif
                break;
            }
        }
        if (EngMisStr[j] == NULL) {
#ifdef FIXIT_CSII //	ajw Added Aftermath installed checks (before, it was assumed). Added officialness check.
            //	Add mission if it's available to us.
            if (!Session.Scenarios[i]->Get_Official()
                || !((Is_Mission_Counterstrike((char*)(Session.Scenarios[i]->Get_Filename()))
                      && !Is_Counterstrike_Installed())
                     || (Is_Mission_Aftermath((char*)(Session.Scenarios[i]->Get_Filename()))
                         && !Is_Aftermath_Installed())))
#endif
                scenariolist.Add_Item(Session.Scenarios[i]->Description());
        }
    }

    Session.Options.ScenarioIndex = 0; // 1st scenario is selected

    /*........................................................................
    Init random-number generator, & create a seed to be used for all random
    numbers from here on out
    ........................................................................*/
#ifdef FIXIT_RANDOM_GAME
    srand((unsigned)time(NULL));
    Seed = rand();
#else
//	randomize();
//	Seed = rand();
#endif

    /*........................................................................
    Init version number clipping system
    ........................................................................*/
    VerNum.Init_Clipping();
    Load_Title_Page(true);
    CCPalette.Set();

    /*
    ---------------------------- Processing loop -----------------------------
    */
    theirresponsetime = 10000; // initialize to an invalid value
    timingtime = lastmsgtime = lastredrawtime = TickCount;

    while (process) {
        /*
        ........................ Invoke game callback .........................
        */
        Call_Back();

        /*
        ** If we have just received input focus again after running in the background then
        ** we need to redraw.
        */
        if (AllSurfaces.SurfacesRestored) {
            AllSurfaces.SurfacesRestored = false;
            display = REDRAW_ALL;
        }

        /*
        ...................... Refresh display if needed ......................
        */
        if (display) {
            if (housebtn.IsDropped) {
                housebtn.Collapse();
                display = REDRAW_BACKGROUND;
            }
            Hide_Mouse();

            /*
            .................. Redraw backgound & dialog box ...................
            */
            if (display >= REDRAW_BACKGROUND) {
                Dialog_Box(d_dialog_x, d_dialog_y, d_dialog_w, d_dialog_h);

                // init font variables

                Fancy_Text_Print(TXT_NONE, 0, 0, scheme, TBLACK, TPF_CENTER | TPF_TEXT);

                /*...............................................................
                Dialog & Field labels
                ...............................................................*/
                Fancy_Text_Print(TXT_YOUR_NAME,
                                 d_name_x + (d_name_w / 2),
                                 d_name_y - d_txt6_h,
                                 scheme,
                                 TBLACK,
                                 TPF_CENTER | TPF_TEXT);

                Fancy_Text_Print(TXT_SIDE_COLON,
                                 d_house_x + (d_house_w / 2),
                                 d_house_y - d_txt6_h,
                                 scheme,
                                 TBLACK,
                                 TPF_CENTER | TPF_TEXT);
                Fancy_Text_Print(TXT_COLOR_COLON,
                                 d_dialog_x + ((d_dialog_w / 4) * 3),
                                 d_color_y - d_txt6_h,
                                 scheme,
                                 TBLACK,
                                 TPF_CENTER | TPF_TEXT);

                Fancy_Text_Print(TXT_EASY, difficulty.X, difficulty.Y - 8 * factor, scheme, TBLACK, TPF_TEXT);
                Fancy_Text_Print(TXT_HARD,
                                 difficulty.X + difficulty.Width,
                                 difficulty.Y - 8 * factor,
                                 scheme,
                                 TBLACK,
                                 TPF_RIGHT | TPF_TEXT);
                Fancy_Text_Print(TXT_NORMAL,
                                 difficulty.X + difficulty.Width / 2,
                                 difficulty.Y - 8 * factor,
                                 scheme,
                                 TBLACK,
                                 TPF_CENTER | TPF_TEXT);
                Fancy_Text_Print(TXT_SCENARIOS,
                                 d_scenariolist_x + (d_scenariolist_w / 2),
                                 d_scenariolist_y - d_txt6_h,
                                 scheme,
                                 TBLACK,
                                 TPF_CENTER | TPF_TEXT);
                Fancy_Text_Print(TXT_COUNT, d_count_x - 2, d_count_y, scheme, TBLACK, TPF_TEXT | TPF_RIGHT);
                Fancy_Text_Print(TXT_LEVEL, d_level_x - 2, d_level_y, scheme, TBLACK, TPF_TEXT | TPF_RIGHT);
                Fancy_Text_Print(TXT_CREDITS_COLON, d_credits_x - 2, d_credits_y, scheme, TBLACK, TPF_TEXT | TPF_RIGHT);
                Fancy_Text_Print(TXT_AI_PLAYERS_COLON,
                                 d_aiplayers_x - 2 * factor,
                                 d_aiplayers_y,
                                 scheme,
                                 TBLACK,
                                 TPF_TEXT | TPF_RIGHT);
            }

            /*..................................................................
            Draw the color boxes
            ..................................................................*/
            if (display >= REDRAW_COLORS) {
                for (i = 0; i < MAX_MPLAYER_COLORS; i++) {
                    LogicPage->Fill_Rect(cbox_x[i] + 1,
                                         d_color_y + 1,
                                         cbox_x[i] + 1 + d_color_w - 2,
                                         d_color_y + 1 + d_color_h - 2,
                                         ColorRemaps[i].Box);

                    if (i == Session.ColorIdx) {
                        Draw_Box(cbox_x[i], d_color_y, d_color_w, d_color_h, BOXSTYLE_DOWN, false);
                    } else {
                        Draw_Box(cbox_x[i], d_color_y, d_color_w, d_color_h, BOXSTYLE_RAISED, false);
                    }
                }
            }

            //..................................................................
            // Update game parameter labels
            //..................................................................
            if (display >= REDRAW_PARMS) {
                sprintf(staticcountbuff, "%d", Session.Options.UnitCount);
                staticcount.Set_Text(staticcountbuff);
                staticcount.Draw_Me();

                if (BuildLevel <= MPLAYER_BUILD_LEVEL_MAX) {
                    sprintf(staticlevelbuff, "%d ", BuildLevel);
                } else {
                    sprintf(staticlevelbuff, "**");
                }
                staticlevel.Set_Text(staticlevelbuff);
                staticlevel.Draw_Me();

                sprintf(staticcreditsbuff, "%d", Session.Options.Credits);
                staticcredits.Set_Text(staticcreditsbuff);
                staticcredits.Draw_Me();

                sprintf(staticaibuff, "%d", Session.Options.AIPlayers);
                staticai.Set_Text(staticaibuff);
                staticai.Draw_Me();
            }

            /*
            .......................... Redraw buttons ..........................
            */
            if (display >= REDRAW_BUTTONS) {
                commands->Flag_List_To_Redraw();
                commands->Draw_All();
            }

            Show_Mouse();
            display = REDRAW_NONE;
        }

        /*
        ........................... Get user input ............................
        */
        messages_have_focus = Session.Messages.Has_Edit_Focus();
        bool droplist_is_dropped = housebtn.IsDropped;
        input = commands->Input();

        /*
        ** Redraw everything if the droplist collapsed
        */
        if (droplist_is_dropped && !housebtn.IsDropped) {
            display = REDRAW_BACKGROUND;
        }

        if (input & KN_BUTTON) {
            if (housebtn.IsDropped) {
                housebtn.Collapse();
                display = REDRAW_BACKGROUND;
            }
        }

        /*
        ---------------------------- Process input ----------------------------
        */
        switch (input) {
        /*------------------------------------------------------------------
        User clicks on a color button
        ------------------------------------------------------------------*/
        case KN_LMOUSE:
            if (WWKeyboard->MouseQX > cbox_x[0] && WWKeyboard->MouseQX < (cbox_x[MAX_MPLAYER_COLORS - 1] + d_color_w)
                && WWKeyboard->MouseQY > d_color_y && WWKeyboard->MouseQY < (d_color_y + d_color_h)) {

                Session.PrefColor = (PlayerColorType)((WWKeyboard->MouseQX - cbox_x[0]) / d_color_w);
                Session.ColorIdx = Session.PrefColor;
                if (display < REDRAW_COLORS)
                    display = REDRAW_COLORS;

                name_edt.Set_Color(
                    &ColorRemaps[(Session.ColorIdx == PCOLOR_DIALOG_BLUE) ? PCOLOR_REALLY_BLUE : Session.ColorIdx]);
                name_edt.Flag_To_Redraw();
                Session.Messages.Set_Edit_Color((Session.ColorIdx == PCOLOR_DIALOG_BLUE) ? PCOLOR_REALLY_BLUE
                                                                                         : Session.ColorIdx);
                strcpy(Session.Handle, namebuf);
                transmit = true;
                changed = true;
                if (housebtn.IsDropped) {
                    housebtn.Collapse();
                    display = REDRAW_BACKGROUND;
                }
            }
            break;

        /*------------------------------------------------------------------
        User edits the name field; retransmit new game options
        ------------------------------------------------------------------*/
        case (BUTTON_NAME | KN_BUTTON):
            if (housebtn.IsDropped) {
                housebtn.Collapse();
                display = REDRAW_BACKGROUND;
            }
            strcpy(Session.Handle, namebuf);
            transmit = true;
            changed = true;
            break;

        case (BUTTON_HOUSE | KN_BUTTON):
            Session.House = HousesType(housebtn.Current_Index() + HOUSE_USSR);
            strcpy(Session.Handle, namebuf);
            display = REDRAW_BACKGROUND;
            transmit = true;
            break;

            /*------------------------------------------------------------------
            New Scenario selected.
            ------------------------------------------------------------------*/
        case (BUTTON_SCENARIOLIST | KN_BUTTON):
            if (housebtn.IsDropped) {
                housebtn.Collapse();
                display = REDRAW_BACKGROUND;
            }
            if (scenariolist.Current_Index() != Session.Options.ScenarioIndex) {
                Session.Options.ScenarioIndex = scenariolist.Current_Index();
                strcpy(Session.Handle, namebuf);
                transmit = true;
            }
            break;

        /*------------------------------------------------------------------
        User adjusts max # units
        ------------------------------------------------------------------*/
        case (BUTTON_COUNT | KN_BUTTON):
            Session.Options.UnitCount = countgauge.Get_Value() + SessionClass::CountMin[Session.Options.Bases];
            if (display < REDRAW_PARMS)
                display = REDRAW_PARMS;
            if (housebtn.IsDropped) {
                housebtn.Collapse();
                display = REDRAW_BACKGROUND;
            }
            transmit = true;
            break;

        /*------------------------------------------------------------------
        User adjusts build level
        ------------------------------------------------------------------*/
        case (BUTTON_LEVEL | KN_BUTTON):
            BuildLevel = levelgauge.Get_Value() + 1;
            if (BuildLevel > MPLAYER_BUILD_LEVEL_MAX) // if it's pegged, max it out
                BuildLevel = MPLAYER_BUILD_LEVEL_MAX;
            if (display < REDRAW_PARMS)
                display = REDRAW_PARMS;
            if (housebtn.IsDropped) {
                housebtn.Collapse();
                display = REDRAW_BACKGROUND;
            }
            transmit = true;
            break;

        /*------------------------------------------------------------------
        User adjusts max # units
        ------------------------------------------------------------------*/
        case (BUTTON_CREDITS | KN_BUTTON):
            Session.Options.Credits = creditsgauge.Get_Value();
            Session.Options.Credits = ((Session.Options.Credits + 250) / 500) * 500;
            if (display < REDRAW_PARMS)
                display = REDRAW_PARMS;
            if (housebtn.IsDropped) {
                housebtn.Collapse();
                display = REDRAW_BACKGROUND;
            }
            transmit = true;
            break;

        //..................................................................
        //	User adjusts # of AI players
        //..................................................................
        case (BUTTON_AIPLAYERS | KN_BUTTON): {
            Session.Options.AIPlayers = aiplayersgauge.Get_Value();
            int humans = 1; // One humans.
            Session.Options.AIPlayers += 1;
            if (Session.Options.AIPlayers + humans >= Rule.MaxPlayers) { // if it's pegged, max it out
                Session.Options.AIPlayers = Rule.MaxPlayers - humans;
                aiplayersgauge.Set_Value(Session.Options.AIPlayers - 1);
            }
            transmit = true;
            if (display < REDRAW_PARMS)
                display = REDRAW_PARMS;

            if (housebtn.IsDropped) {
                housebtn.Collapse();
                display = REDRAW_BACKGROUND;
            }

            break;
        }

        //------------------------------------------------------------------
        // Toggle-able options:
        // If 'Bases' gets toggled, we have to change the range of the
        // UnitCount slider.
        // Also, if Tiberium gets toggled, we have to set the flags
        // in SpecialClass.
        //------------------------------------------------------------------
        case (BUTTON_OPTIONS | KN_BUTTON):
            if (Session.Options.Bases != optionlist.Is_Checked(0)) {
                Session.Options.Bases = optionlist.Is_Checked(0);
                if (Session.Options.Bases) {
                    Session.Options.UnitCount =
                        Fixed_To_Cardinal(SessionClass::CountMax[1] - SessionClass::CountMin[1],
                                          Cardinal_To_Fixed(SessionClass::CountMax[0] - SessionClass::CountMin[0],
                                                            Session.Options.UnitCount - SessionClass::CountMin[0]))
                        + SessionClass::CountMin[1];
                } else {
                    Session.Options.UnitCount =
                        Fixed_To_Cardinal(SessionClass::CountMax[0] - SessionClass::CountMin[0],
                                          Cardinal_To_Fixed(SessionClass::CountMax[1] - SessionClass::CountMin[1],
                                                            Session.Options.UnitCount - SessionClass::CountMin[1]))
                        + SessionClass::CountMin[0];
                }
                countgauge.Set_Maximum(SessionClass::CountMax[Session.Options.Bases]
                                       - SessionClass::CountMin[Session.Options.Bases]);
                countgauge.Set_Value(Session.Options.UnitCount - SessionClass::CountMin[Session.Options.Bases]);
            }
            Session.Options.Tiberium = optionlist.Is_Checked(1);
            Special.IsTGrowth = Session.Options.Tiberium;
            Rule.IsTGrowth = Session.Options.Tiberium;
            Special.IsTSpread = Session.Options.Tiberium;
            Rule.IsTSpread = Session.Options.Tiberium;

            Session.Options.Goodies = optionlist.Is_Checked(2);
            Special.IsShadowGrow = optionlist.Is_Checked(3);
            Special.IsCaptureTheFlag = optionlist.Is_Checked(4);

            transmit = true;
            if (display < REDRAW_PARMS)
                display = REDRAW_PARMS;
            if (housebtn.IsDropped) {
                housebtn.Collapse();
                display = REDRAW_BACKGROUND;
            }
            break;

        /*------------------------------------------------------------------
        OK: exit loop with true status
        ------------------------------------------------------------------*/
        case (BUTTON_LOAD | KN_BUTTON):
        case (BUTTON_OK | KN_BUTTON):
            if (housebtn.IsDropped) {
                housebtn.Collapse();
                display = REDRAW_BACKGROUND;
            }
            //
            // make sure we got a game options packet from the other player
            //
            if (Session.Scenarios.Count() > 0) {
                if (gameoptions) {
                    rc = true;
                    process = false;

                    // force transmitting of game options packet one last time

                    transmit = true;
                    transmittime = 0;
                } else {
                    WWMessageBox().Process(TXT_ONLY_ONE, TXT_OOPS, NULL);
                    display = REDRAW_ALL;
                }
            }
            if (input == (BUTTON_LOAD | KN_BUTTON))
                load_game = true;
            break;

        /*------------------------------------------------------------------
        CANCEL: send a SIGN_OFF, bail out with error code
        ------------------------------------------------------------------*/
        case (KN_ESC):
        case (BUTTON_CANCEL | KN_BUTTON):
            if (housebtn.IsDropped) {
                housebtn.Collapse();
                display = REDRAW_BACKGROUND;
            }
            process = false;
            rc = false;
            break;

        /*------------------------------------------------------------------
        Default: manage the inter-player messages
        ------------------------------------------------------------------*/
        default:
            break;
        }

        /*---------------------------------------------------------------------
        Detect editing of the name buffer, transmit new values to players
        ---------------------------------------------------------------------*/
        if (strcmp(namebuf, Session.Handle)) {
            strcpy(Session.Handle, namebuf);
            transmit = true;
            changed = true;
        }

        Frame_Limiter();
    }

    /*------------------------------------------------------------------------
    Prepare to load the scenario
    ------------------------------------------------------------------------*/
    if (rc) {
        Session.NumPlayers = 1;

        Scen.Scenario = Session.Options.ScenarioIndex;
        strcpy(Scen.ScenarioName, Session.Scenarios[Session.Options.ScenarioIndex]->Get_Filename());

        /*.....................................................................
        Add both players to the Players vector; the local system is always
        index 0.
        .....................................................................*/
        who = new NodeNameType;
        strcpy(who->Name, namebuf);
        who->Player.House = Session.House;
        who->Player.Color = Session.ColorIdx;
        who->Player.ProcessTime = -1;
        Session.Players.Add(who);

        /*
        **	Fetch the difficulty setting when in skirmish mode.
        */

        int diff = difficulty.Get_Value() * (Rule.IsFineDifficulty ? 1 : 2);
        switch (diff) {
        case 0:
            Scen.CDifficulty = DIFF_HARD;
            Scen.Difficulty = DIFF_EASY;
            break;

        case 1:
            Scen.CDifficulty = DIFF_HARD;
            Scen.Difficulty = DIFF_NORMAL;
            break;

        case 2:
            Scen.CDifficulty = DIFF_NORMAL;
            Scen.Difficulty = DIFF_NORMAL;
            break;

        case 3:
            Scen.CDifficulty = DIFF_EASY;
            Scen.Difficulty = DIFF_NORMAL;
            break;

        case 4:
            Scen.CDifficulty = DIFF_EASY;
            Scen.Difficulty = DIFF_HARD;
            break;
        }
    }

    /*------------------------------------------------------------------------
    Clear all lists
    ------------------------------------------------------------------------*/
    while (scenariolist.Count()) {
        scenariolist.Remove_Item(scenariolist.Get_Item(0));
    }

    /*------------------------------------------------------------------------
    Clean up the list boxes
    ------------------------------------------------------------------------*/
    while (playerlist.Count() > 0) {
        item = (char*)playerlist.Get_Item(0);
        delete[] item;
        playerlist.Remove_Item(item);
    }

    /*------------------------------------------------------------------------
    Remove the chat edit box
    ------------------------------------------------------------------------*/
    Session.Messages.Remove_Edit();

    /*------------------------------------------------------------------------
    Restore screen
    ------------------------------------------------------------------------*/
    Hide_Mouse();
    Load_Title_Page(true);
    Show_Mouse();

    /*------------------------------------------------------------------------
    Save any changes made to our options
    ------------------------------------------------------------------------*/
    if (changed) {
        Session.Write_MultiPlayer_Settings();
    }

    return (rc);
}
#endif // REMASTER_BUILD
