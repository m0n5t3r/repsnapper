/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2010  Kulitorum

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#pragma once
#include <gtkmm.h>

#include <vector>
#include "platform.h"
#include "math.h"                                               // Needed for sqrtf

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include <vmmlib/vmmlib.h>

using namespace std;
using namespace vmml;

/*

Movement Codes

G0 Rapid Motion Implemented - supports X, Y, and Z axes.
G1 Coordinated Motion Implemented - supports X, Y, and Z axes.
G2 Arc � Clockwise Not used by Skienforge
G3 Arc - Counter Clockwise Not used by Skienforge
G4 Dwell Implemented.
G20 Inches as units Implemented.
G21 Millimetres as units Implemented.
G28 Go Home Implemented. (X = -135mm, Y = 100mm, Z = 0mm)
G90 Absolute Positioning Implemented. V1.0.5
G92 Set current as home Implemented V1.0.5

G1

Coordinated Motion

G1 X-53.67 Y52.16 Z0.54 E54.394 F213.333

Go to X=-53.67, Y=52.16, Z=0.54 using SPEED=213.333 mm per minute (3.5mm/sec)
E is distance and is still not implemented in RapMan firmware
Note that if E values (E-code) exists for the G1 the RapMan firmware up to 1.0.5 will not parse it properly and will cause all different problems during print. You can disable E value generation in INSET tab by setting the "Extrusion Distance Format Choice" to "Do Not Add Extrusion Distance".
Special functions (M Codes)

M101 Turn extruder on Forward.
M102 Turn extruder on Reverse. Still to add
M103 Turn extruder off.
M104 S145.0 Set target temperature to 145.0 C.
M105 Custom code for temperature reading. Not used
M106 Turn fan on.
M107 Turn fan off.
M108 S400 Set Extruder speed to S value/10 = 40rpm.

Future functions (M Codes)

M codes that may be introduced for the RepRap
M120 Pgain PWM control values
M121 Igain �
M122 Dgain �
M123 Imax �
M124 Imin �


Special BFB-only M Codes

M220 Turn off AUX V1.0.5
M221 Turn on AUX V1.0.5
M222 Set speed of fast XY moves V1.0.5
M223 Set speed of fast Z moves V1.0.5
M224 Enable extruder motor during fast move
M225 Disable extruder motor during fast move
M226 Pause the printer until ESC key is pressed
M227 S1000 P800 on extruder stop (M103) reverse the extruder stepper for S turns and on extruder start (M101) prepare (push) filament P steps (available from firmware 1.0.8)

Typical File header produced by Skeinforge

G21 millimeter system selection
G90 absolute distance mode
G28 Return to home position
M222 S1024 Manually added to file M222 S500 Set speed of fast XY moves. Default value 500 -V1.0.5
M223 S768 Manually added to file M223 S500 Set speed of fast Z moves. Default value 500 -V1.0.5
M103 Turn extruder OFF.
M105 Custom code for temperature reading � not used by RapMan
M104 S247.0 Set temperature to 145.0 DegC.
G1 X0.0 Y0.0 Z0.0 F480.0 linear interpolation
M101 Turn extruder ON.
Main code follows

*/


enum GCodes{GOTO, DRAWTO,  DWELL, // 0 1 2
	    RAPIDMOTION, COORDINATEDMOTION, COORDINATEDMOTION3D,
	    ARC_CW, ARC_CCW,
	    EXTRUDERON, EXTRUDERONREVERSE, EXTRUDEROFF, //  6 7
	    ARCCLOCKWISE, ARCCOUNTERCLOCKWISE,  // 8 9
	    MILLIMETERSASUNITS,	INCHESASUNITS,    // 10 11
	    GOHOME,  GOHOMEVIAINTERMEDIATEPOINT, // 12 13
	    ABSOLUTEPOSITIONING, RELATIVEPOSITIONING, // 14 15
	    SETCURRENTPOS, SELECTEXTRUDER, ZMOVE, SETSPEED,  // 16 - 19
	    FANON,FANOFF,
	    ASKTEMP};
const string MCODES[] = {"G92", "", "",
			 "G0", "G1", "G1",
			 "G2", "G3",
			 "M101", "M102", "M103", // eon erev eoff
			 "", "", // arcs
			 "G21", "G20", // mm in
			 "G28", "",
			 "G90", "G91", // abs. rel. pos
			 "G92", "T0", "G1", "G1" ,
			 "M106", "M107",
			 "M105"}; // temp?


class Model;
class ViewProgress;

class Command
{
public:
	Command(){where.x=where.y=where.z=e=-1.0;f=0.0;};
	Command(GCodes code, const Vector3d where=Vector3d(0,0,0), double E=0, double F=0);
	Command(string gcodeline, Vector3d defaultpos);
	Command(const Command &rhs);
	GCodes Code;
	Vector3d where;
	Vector3d arcIJK; // I,J,K (dx, dy, dz) 
	double f,e; // Feedrates f=speed, e=extrusion to preform while moving (Pythagoras)
	string comment;
	void draw(Vector3d &lastPos, guint linewidth, Vector4f color) const;
	void draw(Vector3d &lastPos) const;
	string GetGCodeText(Vector3d &LastPos, double &lastE, bool incrementalEcode) const;
	string info() const;
};

class GCodeImpl;
class RepRapSerial;

class GCodeIter
{
  Glib::RefPtr<Gtk::TextBuffer> m_buffer;
  Gtk::TextBuffer::iterator m_it;
 public:
  unsigned long m_line_count, m_cur_line;
  GCodeIter (Glib::RefPtr<Gtk::TextBuffer> buffer);
  std::string next_line ();
  bool finished();
  double time_used;
  time_t time_started;
  double time_estimation;
  Command getCurrentCommand(Vector3d defaultwhere);
};

class GCode
{
public:
  GCode();

  void Read  (Model *model, ViewProgress *progress, string filename);
  //void Write (Model *model, string filename);
  void draw  (const Settings &settings, int layer=-1, bool liveprinting=false,
	      int linewidth=3);
  void drawCommands(const Settings &settings, uint start, uint end,
		    bool liveprinting, int linewidth);
  void MakeText(string &GcodeTxt, const string &GcodeStart,
		const string &GcodeLayer, const string &GcodeEnd,
		bool UseIncrementalEcode, bool Use3DGcode,
		double AntioozeDistance, double AntioozeSpeed,
		ViewProgress * progress);
  
  bool append_text (const std::string &line);
  std::string get_text() const;
  void clear();

  std::vector<Command> commands;
  Vector3d Min, Max, Center;

  Glib::RefPtr<Gtk::TextBuffer> buffer;
  GCodeIter *get_iter ();

  double GetTimeEstimation() const;

  vector<unsigned long> layerchanges;
  int getLayerNo(const double z) const;
  int getLayerNo(const unsigned long commandno) const;
  
private:
  unsigned long unconfirmed_blocks;
};
