/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <stdio.h>

#include "../simdebug.h"
#include "../simworld.h"
#include "../simimg.h"

#include "../simtools.h"
#include "../boden/grund.h"
#include "../dataobj/loadsave.h"

#include "simpeople.h"
#include "../besch/fussgaenger_besch.h"

static uint32 const strecke[] = { 6000, 11000, 15000, 20000, 25000, 30000, 35000, 40000 };

static weighted_vector_tpl<const fussgaenger_besch_t*> liste;
stringhashtable_tpl<const fussgaenger_besch_t *> fussgaenger_t::table;


static bool compare_fussgaenger_besch(const fussgaenger_besch_t* a, const fussgaenger_besch_t* b)
{
	/* Gleiches Level - wir f�hren eine k�nstliche, aber eindeutige Sortierung
	 * �ber den Namen herbei. */
	return strcmp(a->get_name(), b->get_name())<0;
}


bool fussgaenger_t::register_besch(const fussgaenger_besch_t *besch)
{
	if(  table.remove(besch->get_name())  ) {
		dbg->warning( "fussgaenger_besch_t::register_besch()", "Object %s was overlaid by addon!", besch->get_name() );
	}
	table.put(besch->get_name(), besch);
	return true;
}


bool fussgaenger_t::alles_geladen()
{
	liste.resize(table.get_count());
	if (table.empty()) {
		DBG_MESSAGE("fussgaenger_t", "No pedestrians found - feature disabled");
	}
	else {
		vector_tpl<const fussgaenger_besch_t*> temp_liste(0);
		stringhashtable_iterator_tpl<const fussgaenger_besch_t *>iter(table);
		while(  iter.next()  ) {
			// just entered them sorted
			temp_liste.insert_ordered( iter.get_current_value(), compare_fussgaenger_besch );
		}
		FOR(vector_tpl<fussgaenger_besch_t const*>, const i, temp_liste) {
			liste.append(i, i->get_gewichtung());
		}
	}
	return true;
}


fussgaenger_t::fussgaenger_t(karte_t *welt, loadsave_t *file)
 : verkehrsteilnehmer_t(welt)
{
	rdwr(file);
	if(besch) {
		welt->sync_add(this);
	}
}


fussgaenger_t::fussgaenger_t(karte_t* const welt, koord3d const pos) :
	verkehrsteilnehmer_t(welt, pos),
	besch(pick_any_weighted(liste))
{
	time_to_life = pick_any(strecke);
	calc_bild();
}



void fussgaenger_t::calc_bild()
{
	set_bild(besch->get_bild_nr(ribi_t::get_dir(get_fahrtrichtung())));
}



void fussgaenger_t::rdwr(loadsave_t *file)
{
	xml_tag_t f( file, "fussgaenger_t" );

	verkehrsteilnehmer_t::rdwr(file);

	if(!file->is_loading()) {
		const char *s = besch->get_name();
		file->rdwr_str(s);
	}
	else {
		char s[256];
		file->rdwr_str(s, lengthof(s));
		besch = table.get(s);
		// unknow pedestrian => create random new one
		if(besch == NULL  &&  !liste.empty()  ) {
			besch = pick_any_weighted(liste);
		}
	}

	if(file->get_version()<89004) {
		time_to_life = pick_any(strecke);
	}
}



// create anzahl pedestrains (if possible)
void fussgaenger_t::erzeuge_fussgaenger_an(karte_t *welt, const koord3d k, int &anzahl)
{
	if (liste.empty()) {
		return;
	}

	const grund_t* bd = welt->lookup(k);
	if (bd) {
		const weg_t* weg = bd->get_weg(road_wt);

		// we do not start on crossings (not overrunning pedestrians please
		if (weg && ribi_t::is_twoway(weg->get_ribi_unmasked())) {
			// we create maximal 4 pedestrians here for performance reasons
			for (int i = 0; i < 4 && anzahl > 0; i++) {
				fussgaenger_t* fg = new fussgaenger_t(welt, k);
				bool ok = welt->lookup(k)->obj_add(fg) != 0;	// 256 limit reached
				if (ok) {
					if (i > 0) {
						// walk a little
						fg->sync_step( (i & 3) * 64 * 24);
					}
					welt->sync_add(fg);
					anzahl--;
				} else {
					// delete it, if we could not put it on the map
					fg->set_flag(ding_t::not_on_map);
					delete fg;
					return; // it is pointless to try again
				}
			}
		}
	}
}


bool fussgaenger_t::sync_step(long delta_t)
{
	time_to_life -= delta_t;

	weg_next += 128*delta_t;
	weg_next -= fahre_basis( weg_next );
	return time_to_life>0;
}
