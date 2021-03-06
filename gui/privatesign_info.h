/*
 * Copyright (c) 1997 - 2003 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef privatesign_info_t_h
#define privatesign_info_t_h

#include "../simconst.h"
#include "thing_info.h"
#include "components/action_listener.h"
#include "components/gui_button.h"
#include "gui_container.h"

class roadsign_t;

/**
 * Info window for factories
 * @author Hj. Malthaner
 */
class privatesign_info_t : public ding_infowin_t, public action_listener_t
{
 private:
	roadsign_t* sign;
	button_t players[PLAYER_UNOWNED];

 public:
	privatesign_info_t(roadsign_t* s);

	/**
	 * Manche Fenster haben einen Hilfetext assoziiert.
	 * @return den Dateinamen f�r die Hilfe, oder NULL
	 * @author Hj. Malthaner
	 */
	const char *get_hilfe_datei() const {return "privatesign_info.txt";}

	/**
	 * This method is called if an action is triggered
	 * @author Hj. Malthaner
	 *
	 * Returns true, if action is done and no more
	 * components should be triggered.
	 * V.Meyer
	 */
	bool action_triggered( gui_action_creator_t *komp, value_t extra);


	// called, after external change
	void update_data();
};

#endif
