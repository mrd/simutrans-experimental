
#include <cmath>
#include <string>
#include "../../utils/simstring.h"
#include "../../dataobj/tabfile.h"
#include "../vehikel_besch.h"
#include "../sound_besch.h"
#include "../../boden/wege/weg.h"
#include "obj_pak_exception.h"
#include "obj_node.h"
#include "text_writer.h"
#include "xref_writer.h"
#include "imagelist_writer.h"
#include "imagelist2d_writer.h"
#include "imagelist3d_writer.h"
#include "get_waytype.h"
#include "vehicle_writer.h"

using std::string;

/**
 * Calculate numeric engine type from engine type string
 */
static vehikel_besch_t::engine_t get_engine_type(char const* const engine_type)
{
	vehikel_besch_t::engine_t uv8 = vehikel_besch_t::diesel;

	if (!STRICMP(engine_type, "diesel")) {
		uv8 = vehikel_besch_t::diesel;
	} else if (!STRICMP(engine_type, "electric")) {
		uv8 = vehikel_besch_t::electric;
	} else if (!STRICMP(engine_type, "steam")) {
		uv8 = vehikel_besch_t::steam;
	} else if (!STRICMP(engine_type, "bio")) {
		uv8 = vehikel_besch_t::bio;
	} else if (!STRICMP(engine_type, "sail")) {
		uv8 = vehikel_besch_t::sail;
	} else if (!STRICMP(engine_type, "fuel_cell")) {
		uv8 = vehikel_besch_t::fuel_cell;
	} else if (!STRICMP(engine_type, "hydrogene")) {
		uv8 = vehikel_besch_t::hydrogene;
	} else if (!STRICMP(engine_type, "battery")) {
		uv8 = vehikel_besch_t::battery;
	} else if (!STRICMP(engine_type, "unknown")) {
		uv8 = vehikel_besch_t::unknown;
	}

	// printf("Engine type %s -> %d\n", engine_type, uv8);

	return uv8;
}


/**
 * Writes vehicle node data to file
 *
 * NOTE: The data must be written in _exactly_
 * the same sequence as it is to be read in the
 * relevant reader file. The "total_len" field is
 * the length in bytes of the VHCL node of the
 * pak file. The VHCL node is the first node
 * beneath the header node, and contains all of
 * the _numerical_ information about the vehicle,
 * such as the introduction date, running costs,
 * etc.. Text (including filenames of sound files),
 * and graphics are _not_ part of the VHCL node,
 * and therefore do not count towards total length.
 * Furthermore, the third argument to the node.write
 * method must ascend sequentially with the number 
 * of bytes written so far (up 1 for a uint8, 2 for
 * a uint16, 4 for a uint32 and so forth). Failure
 * to observe these rules will result in data
 * corruption and errors when the pak file is read
 * by the main program.
 * @author of note: jamespetts
 */
void vehicle_writer_t::write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj)
{
	int i;
	uint8  uv8;

	int total_len = 67;

	// prissi: must be done here, since it may affect the len of the header!
	string sound_str = ltrim( obj.get("sound") );
	sint8 sound_id=NO_SOUND;
	if (!sound_str.empty()) {
		// ok, there is some sound
		sound_id = atoi(sound_str.c_str());
		if (sound_id == 0 && sound_str[0] == '0') {
			sound_id = 0;
			sound_str = "";
		} else if (sound_id != 0) {
			// old style id
			sound_str = "";
		}
		if (!sound_str.empty()) {
			sound_id = LOAD_SOUND;
			total_len += sound_str.size() + 1;
		}
	}

	obj_node_t	node(this, total_len, &parent);

	write_head(fp, node, obj);


	// Hajo: version number
	// Hajo: Version needs high bit set as trigger -> this is required
	//       as marker because formerly nodes were unversionend
	uint16 version = 0x8008; 
	
	// This is the overlay flag for Simutrans-Experimental
	// This sets the *second* highest bit to 1. 
	version |= EXP_VER;

	// Finally, this is the experimental version number. This is *added*
	// to the standard version number, to be subtracted again when read.
	// Start at 0x100 and increment in hundreds (hex).
	version += 0x700;

	node.write_uint16(fp, version, 0);


	// Hajodoc: Price of this vehicle in cent
	// Hajoval: int
	uint32 cost = obj.get_int("cost", 0);
	node.write_uint32(fp, cost, 2);


	// Hajodoc: Payload of this vehicle
	// Hajoval: int
	uint16 payload = obj.get_int("payload", 0);
	node.write_uint16(fp, payload, 6);


	// Hajodoc: Top speed of this vehicle. Must be greater than 0
	// Hajoval: int
	uint16 top_speed = obj.get_int("speed", 0);
	node.write_uint16(fp, top_speed, 8);


	// Hajodoc: Total weight of this vehicle in tonnes
	// Hajoval: int
	uint16 weight = obj.get_int("weight", 0);
	node.write_uint16(fp, weight, 10);


	// Hajodoc: Power of this vehicle in KW
	// Hajoval: int
	uint32 power = obj.get_int("power", 0);
	node.write_uint32(fp, power, 12);


	// Hajodoc: Running costs, given in cent per square
	// Hajoval: int
	uint16 runningcost = obj.get_int("runningcost", 0);
	node.write_uint16(fp, runningcost, 16);


	// Hajodoc: Introduction date (year * 12 + month)
	// Hajoval: int
	uint16 intro  = obj.get_int("intro_year", DEFAULT_INTRO_DATE) * 12;
	intro += obj.get_int("intro_month", 1) - 1;
	node.write_uint16(fp, intro, 18);

	// Hajodoc: retire date (year * 12 + month)
	// Hajoval: int
	uint16 retire = obj.get_int("retire_year", DEFAULT_RETIRE_DATE) * 12;
	retire += obj.get_int("retire_month", 1) - 1;
	node.write_uint16(fp, retire, 20);

	// Hajodoc: Engine gear (power multiplier)
	// Hajoval: int
	uint16 gear = (obj.get_int("gear", 100) * 64) / 100;
	node.write_uint16(fp, gear, 22);

	// Hajodoc: Type of way this vehicle drives on
	// Hajoval: road, track, electrified_track, monorail_track, maglev_track, water
	char const* const waytype_name = obj.get("waytype");
	waytype_t   const waytype      = get_waytype(waytype_name);
	uv8 = waytype != overheadlines_wt ? waytype : track_wt;
	node.write_uint8(fp, uv8, 24);

	// Hajodoc: The freight type
	// Hajoval: string
	const char* freight = obj.get("freight");
	if (!*freight) {
		freight = "None";
	}
	xref_writer_t::instance()->write_obj(fp, node, obj_good, freight, true);
	xref_writer_t::instance()->write_obj(fp, node, obj_smoke, obj.get("smoke"), false);

	// Jetzt kommen die Bildlisten
	// "Now the picture lists" (Google)
	static const char* const dir_codes[] = {
		"s", "w", "sw", "se", "n", "e", "ne", "nw"
	};
	slist_tpl<string> emptykeys;
	slist_tpl<slist_tpl<string> > freightkeys;
	slist_tpl<string> freightkeys_old;
	slist_tpl<slist_tpl<string> > liverykeys_empty;
	slist_tpl<slist_tpl<string> > liverykeys_freight_old;
	slist_tpl<slist_tpl<slist_tpl<string> > > liverykeys_freight;
	string str;

	int  freight_max  = 0;
	int	 livery_max	  = 0;
	bool has_8_images = false;

	// first: find out how many freight and liveries
	for (i = 0; i < 127; i++)
	{
		// Freight with multiple types without liveries
		char buf[40];
		sprintf(buf, "freightimage[%d][%s]", i, dir_codes[0]);
		printf("Reading freightimage[%d][%s]\n", i, dir_codes[0]);
		str = obj.get(buf);
		if(str.empty())
		{
			freight_max += i;
			break;
		}
	}
	
	for (i = 0; i < 127; i++)
	{
		// Freight with multiple types and liveries
		for (int j = 0; j < 127; j++)
		{
			char buf[40];
			sprintf(buf, "freightimage[%d][%s][%d]", i, dir_codes[0], j);
			printf("Reading freightimage[%d][%s][%d]\n", i, dir_codes[0], j);
			str = obj.get(buf);
			if(str.empty())
			{
				freight_max += i;
				livery_max += j;
				goto end;
			}
		}
	}
	
end:

	for (i = 0; i < 127; i++)
	{
		// Liveries without fright / empty images
		char buf[40];
		sprintf(buf, "emptyimage[%s][%d]", dir_codes[0], i);
		printf("Reading emptyimage[%s][%d]\n", dir_codes[0], i);
		str = obj.get(buf);
		if(str.empty())
		{
			livery_max += i;
			break;
		}
	}

	for (i = 0; i < 127; i++)
	{
		// Freight with a single type and liveries
		char buf[40];
		sprintf(buf, "freightimage[%s][%d]", dir_codes[0], i);
		printf("Reading freightimage[%s][%d]\n", dir_codes[0], i);
		str = obj.get(buf);
		if(str.empty())
		{
			livery_max += i;
			break;
		}
	}

	// Freight with a single type and single liveries
	uint8 basic_freight_images = 0;
	for (i = 0; i < 127; i++)
	{
		// Freight with a single type and liveries
		char buf[40];
		sprintf(buf, "freightimage[%s]", dir_codes[0]);
		printf("Reading freightimage[%s]\n", dir_codes[0]);
		str = obj.get(buf);
		if(str.empty())
		{
			basic_freight_images = i;
			break;
		}
	}

	// now load the images strings
	for (i = 0; i < 8; i++) 
	{
		char buf[40];

		// Hajodoc: Empty vehicle image for direction, direction in "s", "w", "sw", "se", asymmetric vehicles need also "n", "e", "ne", "nw"
		if (livery_max == 0) 
		{
			// Empty images without multiple liveries
			sprintf(buf, "emptyimage[%s]", dir_codes[i]);
			str = obj.get(buf);
			if(!str.empty())
			{
				emptykeys.append(str);
				if (i >= 4) 
				{
					has_8_images = true;
				}
			} 
			else 
			{
				// stop when empty string is found
				break;
			}
		}
		else
		{
			// Empty images with multiple liveries
			liverykeys_empty.append(slist_tpl<string>());
			for(int livery = 0; livery < livery_max; livery++)
			{
				sprintf(buf, "emptyimage[%s][%d]", dir_codes[i], livery);
				str = obj.get(buf);
				if(str.empty())
				{
					printf("*** FATAL ***:\nMissing emptyimage[%s][%d]!\n", dir_codes[i], livery);
					fflush(NULL);
					exit(0);
				}
				printf("Appending emptyimage[%s][%d]\n", dir_codes[i], livery);
				liverykeys_empty.at(i).append(str);
			}
		}

		if (freight_max == 0 && livery_max == 0) 
		{
			// a single freight image
			// old style definition - just [direction]
			sprintf(buf, "freightimage[%s]", dir_codes[i]);
			str = obj.get(buf);
			if(!str.empty())
			{
				printf("Appending freightimage[%s]\n", dir_codes[i]);
				freightkeys_old.append(str);
			}
		} 
		else if (freight_max > 0 && livery_max == 0)
		{
			freightkeys.append();
			for(int freight = 0; freight < freight_max; freight++)
			{
				sprintf(buf, "freightimage[%d][%s]", freight, dir_codes[i]);
				str = obj.get(buf);
				if(str.empty())
				{
					printf("*** FATAL ***:\nMissing freightimage[%d][%s]!\n", freight, dir_codes[i]);
					fflush(NULL);
					exit(0);
				}
				printf("Appending freightimage[%d][%s]\n", freight, dir_codes[i]);
				freightkeys.at(i).append(str);
			}
		}
		else if(freight_max == 0 && livery_max > 0)
		{
			// a single freight image
			// old style definition - just [direction]
			// With liveries
			liverykeys_freight_old.append(slist_tpl<string>());
			for(int livery = 0; livery < livery_max; livery++)
			{
				sprintf(buf, "freightimage[%s][%d]", dir_codes[i], livery);
				str = obj.get(buf);
				if(str.empty())
				{
					break;
				}
				printf("Appending freightimage[%s][%d]", dir_codes[i], livery);
				liverykeys_freight_old.at(i).append(str);
			}
		}
		else if (freight_max > 0 && livery_max > 0)
		{
			// Liveries *and* freight
			liverykeys_freight.append(slist_tpl<slist_tpl<string> >());
			for(int livery = 0; livery < livery_max; livery++)
			{
				for(int freight = 0; freight < freight_max; freight++)
				{
					sprintf(buf, "freightimage[%d][%s][%d]", freight, dir_codes[i], livery);
					str = obj.get(buf);
					if(str.empty())
					{
						printf("*** FATAL ***:\nMissing freightimage[%d][%s][%d]!\n", freight, dir_codes[i], livery);
						fflush(NULL);
						exit(0);
					}
					liverykeys_freight.at(i).at(livery).append(str);
					printf("Appending freightimage[%d][%s][%d]", freight, dir_codes[i], livery);
				}
			}
		}
		else
		{
			// This should never be reached.
			printf("*** FATAL ***: Error in code");
			exit(0);
		}
	}

	// prissi: added more error checks
	if (has_8_images && emptykeys.get_count() < 8 && liverykeys_empty.get_count() < 8) 
	{
		printf("*** FATAL ***:\nMissing images (must be either 4 or 8 directions (but %i found)!)\n", emptykeys.get_count() + liverykeys_empty.get_count());
		exit(0);
	}

	if (!(freightkeys_old.empty() || liverykeys_freight_old.empty()) && (emptykeys.get_count() != freightkeys_old.get_count() || liverykeys_empty.get_count() != liverykeys_freight_old.get_count()))
	{
		printf("*** FATAL ***:\nMissing freigthimages (must be either 4 or 8 directions (but %i found)!)\n", freightkeys_old.get_count());
		exit(0);
	}

	if(livery_max == 0)
	{
		// Empty images, no multiple liveries
		imagelist_writer_t::instance()->write_obj(fp, node, emptykeys);
		printf("Writing %d single livery empty images\n", emptykeys.get_count());
	}
	else
	{
		// Empty images, multiple liveries
		imagelist2d_writer_t::instance()->write_obj(fp, node, liverykeys_empty);
		printf("Writing %d multiple livery empty images\n", liverykeys_empty.get_count() *  liverykeys_empty.front().get_count());
	}
	
	if (freight_max > 0 && livery_max == 0) 
	{
		// Multiple freight images, no multiple liveries
		imagelist2d_writer_t::instance()->write_obj(fp, node, freightkeys);
		printf("Writing %d single livery multiple type freight images\n", (freightkeys.get_count() * freightkeys.front().get_count()));
	} 
	else if(freight_max > 0 && livery_max > 0)
	{
		// Multiple frieght images, multiple liveries
		imagelist3d_writer_t::instance()->write_obj(fp, node, liverykeys_freight);
		printf("Writing %d multiple livery multiple type freight images\n", (liverykeys_freight.get_count() * liverykeys_freight.front().get_count() * liverykeys_freight.front().front().get_count()));
	}
	else if(freight_max == 0 && livery_max > 0 && basic_freight_images > 0)
	{
		// Single freight images, multiple liveries
		imagelist2d_writer_t::instance()->write_obj(fp, node, liverykeys_freight_old);
		printf("Writing %d single type multiple livery freight images\n", liverykeys_freight_old.get_count() * liverykeys_freight_old.front().get_count());
	}
	else if(freight_max == 0 && livery_max == 0)
	{
		// Single freight images, no multiple liveries
		if (freightkeys_old.get_count() == emptykeys.get_count()) 
		{
			imagelist_writer_t::instance()->write_obj(fp, node, freightkeys_old);
			printf("Writing %d single type single livery freight images\n", freightkeys_old.get_count());
		} 
		else 
		{
			// really empty list ...
			xref_writer_t::instance()->write_obj(fp, node, obj_imagelist, "", false);
			printf("Writing %d non-images\n", freightkeys_old.get_count());
		}
	}

	//
	// Vorg�nger/Nachfolgerbedingungen
	// "Predecessor / Successor conditions" (Google)
	//
	uint8 besch_vorgaenger = 0;
	bool found;
	do {
		char buf[40];

		// Hajodoc: Constraints for previous vehicles
		// Hajoval: string, "none" means only suitable at front of an convoi
		sprintf(buf, "constraint[prev][%d]", besch_vorgaenger);

		str = obj.get(buf);
		found = !str.empty();
		if (found) {
			if (!STRICMP(str.c_str(), "none")) {
				str = "";
			}
			xref_writer_t::instance()->write_obj(fp, node, obj_vehicle, str.c_str(), false);
			besch_vorgaenger++;
		}
	} while (found);

	uint8 besch_nachfolger = 0;
	bool can_be_at_rear = true;
	do {
		char buf[40];

		// Hajodoc: Constraints for successing vehicles
		// Hajoval: string, "none" to disallow any followers
		sprintf(buf, "constraint[next][%d]", besch_nachfolger);

		str = obj.get(buf);

		found = !str.empty();
		if (found)
		{
			if (!STRICMP(str.c_str(), "none")) 
			{
				str = "";
			}
			if(!STRICMP(str.c_str(), "any"))
			{
				// "Any" should not be specified with anything else.
				can_be_at_rear = false;
				break;
			}
			else
			{
				xref_writer_t::instance()->write_obj(fp, node, obj_vehicle, str.c_str(), false);
				besch_nachfolger++;
			}
		}
	} while (found);

	// Upgrades: these are the vehicle types to which this vehicle
	// can be upgraded. "None" means that it cannot be upgraded. 
	// @author: jamespetts
	uint8 upgrades = 0;
	do
	{
		char buf[40];
		sprintf(buf, "upgrade[%d]", upgrades);
		str = obj.get(buf);
		found = !str.empty();
		if (found)
		{
			if (!STRICMP(str.c_str(), "none"))
			{
				str = "";
			}
			else
			{
				xref_writer_t::instance()->write_obj(fp, node, obj_vehicle, str.c_str(), false);
				upgrades++;
			}
		}
	} while (found);

	// multiple freight image types - define what good uses each index
	// good without index will be an error
	for (i = 0; i <= freight_max; i++)
	{
		char buf[40];
		sprintf(buf, "freightimagetype[%d]", i);
		str = obj.get(buf);
		if (i == freight_max) 
		{
			// check for superflous definitions
			if(!str.empty())
			{
				printf("WARNING: More freightimagetype (%i) than freight_images (%i)!\n", i, freight_max);
				fflush(NULL);
			}
			break;
		}
		if(str.empty())
		{
			printf("*** FATAL ***:\nMissing freightimagetype[%i] for %i freight_images!\n", i, freight_max + 1);
			exit(0);
		}
		xref_writer_t::instance()->write_obj(fp, node, obj_good, str.c_str(), false);
	}

	// multiple liveries - define what liveries use each index
	// liveries without an index will be an error
	for (i = 0; i <= livery_max; i++)
	{
		char buf[128];
		sprintf(buf, "liverytype[%d]", i);
		str = obj.get(buf);
		if (i == livery_max) 
		{
			// check for superflous definitions
			if(!str.empty())
			{
				printf("WARNING: More livery types (%i) than liveries (%i)!\n", i, livery_max);
				fflush(NULL);
			}
			break;
		}
		if(str.empty())
		{
			printf("*** FATAL ***:\nMissing liverytype[%i] for %i liveries!\n", i, livery_max + 1);
			exit(0);
		}
		text_writer_t::instance()->write_obj(fp, node, str.c_str());
	}

	// if no index defined then add default as vehicle good
	// if not using freight images then store zero string
	if (freight_max > 0) 
	{
		xref_writer_t::instance()->write_obj(fp, node, obj_good, freight, false);
	}

	if (livery_max > 0) 
	{
		text_writer_t::instance()->write_obj(fp, node, "default");
	}

	node.write_sint8(fp, sound_id, 25);

	if (waytype == overheadlines_wt) {
		// Hajo: compatibility for old style DAT files
		uv8 = vehikel_besch_t::electric;
	} else {
		const char* engine_type = obj.get("engine_type");
		uv8 = get_engine_type(engine_type);
	}
	node.write_uint8(fp, uv8, 26);

	// the length (default 8)
	uint8 length = obj.get_int("length", 8);
	node.write_uint8(fp, length, 27);

	node.write_sint8(fp, besch_vorgaenger, 28);
	node.write_sint8(fp, besch_nachfolger, 29);
	node.write_uint8(fp, (uint8) freight_max, 30);

	// Whether this is a tilting train
	// int
	//@author: jamespetts
	uint8 tilting = (obj.get_int("is_tilting", 0));
	node.write_uint8(fp, tilting, 31);

	// Way constraints
	// One byte for permissive, one byte for prohibitive.
	// Therefore, 8 possible constraints of each type.
	// Permissive: way allows vehicles with matching constraint:
	// vehicles not allowed on any other sort of way. Vehicles
	// without that constraint also allowed on the way.
	// Prohibitive: way allows only vehicles with matching constraint:
	// vehicles with matching constraint allowed on other sorts of way.
	// @author: jamespetts
	
	uint8 permissive_way_constraints = 0;
	uint8 prohibitive_way_constraints = 0;
	char buf_permissive[60];
	char buf_prohibitive[60];
	//Read the values from a file, and put them into an array.
	for(uint8 i = 0; i < 8; i++)
	{
		sprintf(buf_permissive, "way_constraint_permissive[%d]", i);
		sprintf(buf_prohibitive, "way_constraint_prohibitive[%d]", i);
		uint8 tmp_permissive = (obj.get_int(buf_permissive, 255));
		uint8 tmp_prohibitive = (obj.get_int(buf_prohibitive, 255));
		
		//Compress values into a single byte using bitwise OR.
		if(tmp_permissive < 8)
		{
			permissive_way_constraints = (tmp_permissive > 0) ? permissive_way_constraints | (uint8)pow(2, (double)tmp_permissive) : permissive_way_constraints | 1;
		}
		if(tmp_prohibitive < 8)
		{
			prohibitive_way_constraints = (tmp_prohibitive > 0) ? prohibitive_way_constraints | (uint8)pow(2, (double)tmp_prohibitive) : prohibitive_way_constraints | 1;
		}
	}
	node.write_uint8(fp, permissive_way_constraints, 32);
	node.write_uint8(fp, prohibitive_way_constraints, 33);

	// Catering level. 0 = no catering. 
	// Higher numbers, better catering.
	// Catering boosts passenger revenue.
	// @author: jamespetts
	uint8 catering_level = (obj.get_int("catering_level", 0));
	node.write_uint8(fp, catering_level, 34);		

	//Reverseing settings.
	//@author: jamespetts

	// Bidirectional: vehicle can travel backwards without turning around.
	// Function is disabled for road and air vehicles.
	uint8 bidirectional = (obj.get_int("bidirectional", 0));
	node.write_uint8(fp, bidirectional, 35);

	// Can lead from rear: train can run backwards without turning around.
	uint8 can_lead_from_rear = (obj.get_int("can_lead_from_rear", 0));
	node.write_uint8(fp, can_lead_from_rear, 36);

	// Passenger comfort rating - affects revenue on longer journies.
	//@author: jamespetts
	uint8 comfort = (obj.get_int("comfort", 100));
	node.write_uint8(fp, comfort, 37);

	// Overcrowded capacity - can take this much *in addition to* normal capacity,
	// but revenue will be lower and dwell times higher. Mainly for passengers.
	//@author: jamespetts
	uint16 overcrowded_capacity = (obj.get_int("overcrowded_capacity", 0));
	node.write_uint8(fp, overcrowded_capacity, 38);

	// The time in ms that it takes the vehicle to load and unload at stations (i.e.,  
	// the dwell time). The default is 2,000 because that is the value used in 
	// Simutrans-Standard.
	//@author: jamespetts

	uint16 default_loading_time;

	switch(waytype)
	{
		default:	
		case tram_wt:
		case road_wt:
			default_loading_time = 2000;
			break;

		case monorail_wt:
		case maglev_wt:
		case narrowgauge_wt:
		case track_wt:
			default_loading_time = 4000;
			break;

		case water_wt:
			default_loading_time = 20000;
			break;

		case air_wt:
			default_loading_time = 30000;
			break;
	}
	/** 
	 * This is the old system for storing
	 * journey times. It is retained only
	 * for backwards compatibility. Journey
	 * times are now (10.0 and higher)
	 * stored as seconds, and converted to
	 * ticks when set_scale() is called.
	 * @author: jamespetts
	 */
	uint16 loading_time = (obj.get_int("loading_time", default_loading_time));
	node.write_uint16(fp, loading_time, 40);

	// Upgrading settings
	//@author: jamespetts

	node.write_sint8(fp, upgrades, 42);

	// This is the cost of upgrading to this vehicle, rather than buying it new.
	// By default, the cost is the same as a new purchase.
	uint32 upgrade_price = (obj.get_int("upgrade_price", cost));
	node.write_uint32(fp, upgrade_price, 43);

	// If this is set to true (is read as a bool), this will only be able to be purchased
	// as an upgrade to another vehicle, not as a new vehicle.
	uint8 available_only_as_upgrade = (obj.get_int("available_only_as_upgrade", 0));
	node.write_uint8(fp, available_only_as_upgrade, 47);

	// Fixed monthly maintenance costs
	// @author: jamespetts
	uint32 fixed_maintenance = obj.get_int("fixed_maintenance", 0);
	node.write_uint32(fp, fixed_maintenance, 48);

	// Tractive effort
	// @author: jamespetts
	uint16 tractive_effort = obj.get_int("tractive_effort", 0);
	node.write_uint16(fp, tractive_effort, 52);

	// Air resistance
	// @author: jamespetts & Bernd Gabriel
	uint16 air_default;
	switch(waytype)
	{
		default:
		case road_wt:
			air_default = 252; //2.52 when read
			break;
		case track_wt:
		case tram_wt:
		case monorail_wt:
		case narrowgauge_wt:
			air_default = 1300; //13 when read
			break;
		case water_wt:
			air_default = 2500; //25 when read
			break;
		case maglev_wt:		
			air_default = 1000; //10 when read
			break;
		case air_wt:
			air_default = 100; //1 when read
	};

	uint16 air_resistance_hundreds = obj.get_int("air_resistance", air_default);
	node.write_uint16(fp, air_resistance_hundreds, 54);

	node.write_uint8(fp, (uint8)can_be_at_rear, 56);

	// Obsolescence. Zeros indicate that simuconf.tab values should be used.
	// @author: jamespetts
	uint16 increase_maintenance_after_years = obj.get_int("increase_maintenance_after_years", 0);
	node.write_uint16(fp, increase_maintenance_after_years, 57);

	uint16 increase_maintenance_by_percent = obj.get_int("increase_maintenance_by_percent", 0);
	node.write_uint16(fp, increase_maintenance_by_percent, 59);

	uint8 years_before_maintenance_max_reached = obj.get_int("years_before_maintenance_max_reached", 0);
	node.write_uint8(fp, years_before_maintenance_max_reached, 61);

	node.write_uint8(fp, (uint8) livery_max, 62);

	/**
	 * The loading times (minimum and maximum) of this
	 * vehicle in seconds. These are converted to ticks
	 * after being read in simworld.cc's set_scale()
	 * method. Using these values is preferable to using
	 * the old "loading_time", which sets the ticks
	 * directly and therefore bypasses the scale. A value
	 * of 65535, the default, indicates that this value
	 * has not been set manually, and reverts to the
	 * default loading_time. This retains backwards
	 * compatibility with previous versions of paksets. 
	 * @author: jamespetts, August 2011
	 */
	uint16 min_loading_time = obj.get_int("min_loading_time", 65535);
	node.write_uint16(fp, min_loading_time, 63);
	uint16 max_loading_time = obj.get_int("max_loading_time", 65535);
	node.write_uint16(fp, max_loading_time, 65);

	sint8 sound_str_len = sound_str.size();
	if (sound_str_len > 0) {
		node.write_sint8  (fp, sound_str_len, 67);
		node.write_data_at(fp, sound_str.c_str(),     68, sound_str_len);
	}

	node.write(fp);
}
