#ifdef EXTERN_UI_CVAR
	#define UI_CVAR( vmCvar, cvarName, defaultString, cvarFlags ) extern vmCvar_t vmCvar;
#endif

#ifdef DECLARE_UI_CVAR
	#define UI_CVAR( vmCvar, cvarName, defaultString, cvarFlags ) vmCvar_t vmCvar;
#endif

#ifdef UI_CVAR_LIST
	#define UI_CVAR( vmCvar, cvarName, defaultString, cvarFlags ) { & vmCvar, cvarName, defaultString, cvarFlags },
#endif

UI_CVAR( ui_ffa_fraglimit, "ui_ffa_fraglimit", "20", CVAR_ARCHIVE )
UI_CVAR( ui_ffa_timelimit, "ui_ffa_timelimit", "0", CVAR_ARCHIVE )

UI_CVAR( ui_tourney_fraglimit, "ui_tourney_fraglimit", "0", CVAR_ARCHIVE )
UI_CVAR( ui_tourney_timelimit, "ui_tourney_timelimit", "15", CVAR_ARCHIVE )

UI_CVAR( ui_team_fraglimit, "ui_team_fraglimit", "0", CVAR_ARCHIVE )
UI_CVAR( ui_team_timelimit, "ui_team_timelimit", "20", CVAR_ARCHIVE )
UI_CVAR( ui_team_friendly, "ui_team_friendly",  "1", CVAR_ARCHIVE )

UI_CVAR( ui_ctf_capturelimit, "ui_ctf_capturelimit", "8", CVAR_ARCHIVE )
UI_CVAR( ui_ctf_timelimit, "ui_ctf_timelimit", "30", CVAR_ARCHIVE )
UI_CVAR( ui_ctf_friendly, "ui_ctf_friendly",  "0", CVAR_ARCHIVE )

UI_CVAR( ui_1fctf_capturelimit, "ui_1fctf_capturelimit", "8", CVAR_ARCHIVE )
UI_CVAR( ui_1fctf_timelimit, "ui_1fctf_timelimit", "30", CVAR_ARCHIVE )
UI_CVAR( ui_1fctf_friendly, "ui_1fctf_friendly",  "0", CVAR_ARCHIVE )

UI_CVAR( ui_overload_capturelimit, "ui_overload_capturelimit", "8", CVAR_ARCHIVE )
UI_CVAR( ui_overload_timelimit, "ui_overload_timelimit", "30", CVAR_ARCHIVE )
UI_CVAR( ui_overload_friendly, "ui_overload_friendly",  "0", CVAR_ARCHIVE )

UI_CVAR( ui_harvester_capturelimit, "ui_harvester_capturelimit", "20", CVAR_ARCHIVE )
UI_CVAR( ui_harvester_timelimit, "ui_harvester_timelimit", "30", CVAR_ARCHIVE )
UI_CVAR( ui_harvester_friendly, "ui_harvester_friendly",  "0", CVAR_ARCHIVE )

UI_CVAR( ui_elimination_capturelimit, "ui_elimination_capturelimit", "8", CVAR_ARCHIVE )
UI_CVAR( ui_elimination_timelimit, "ui_elimination_timelimit", "20", CVAR_ARCHIVE )

UI_CVAR( ui_ctf_elimination_capturelimit, "ui_ctf_elimination_capturelimit", "8", CVAR_ARCHIVE )
UI_CVAR( ui_ctf_elimination_timelimit, "ui_ctf_elimination_timelimit", "30", CVAR_ARCHIVE )

UI_CVAR( ui_lms_fraglimit, "ui_lms_fraglimit", "20", CVAR_ARCHIVE )
UI_CVAR( ui_lms_timelimit, "ui_lms_timelimit", "0", CVAR_ARCHIVE )

UI_CVAR( ui_dd_capturelimit, "ui_dd_capturelimit", "8", CVAR_ARCHIVE )
UI_CVAR( ui_dd_timelimit, "ui_dd_timelimit", "30", CVAR_ARCHIVE )
UI_CVAR( ui_dd_friendly, "ui_dd_friendly",  "0", CVAR_ARCHIVE )

UI_CVAR( ui_dom_capturelimit, "ui_dom_capturelimit", "500", CVAR_ARCHIVE )
UI_CVAR( ui_dom_timelimit, "ui_dom_timelimit", "30", CVAR_ARCHIVE )
UI_CVAR( ui_dom_friendly, "ui_dom_friendly",  "0", CVAR_ARCHIVE )

UI_CVAR( ui_arenasFile, "g_arenasFile", "", CVAR_INIT|CVAR_ROM )
UI_CVAR( ui_botsFile, "g_botsFile", "", CVAR_INIT|CVAR_ROM )
UI_CVAR( ui_spScores1, "g_spScores1", "", CVAR_ARCHIVE | CVAR_ROM )
UI_CVAR( ui_spScores2, "g_spScores2", "", CVAR_ARCHIVE | CVAR_ROM )
UI_CVAR( ui_spScores3, "g_spScores3", "", CVAR_ARCHIVE | CVAR_ROM )
UI_CVAR( ui_spScores4, "g_spScores4", "", CVAR_ARCHIVE | CVAR_ROM )
UI_CVAR( ui_spScores5, "g_spScores5", "", CVAR_ARCHIVE | CVAR_ROM )
UI_CVAR( ui_spAwards, "g_spAwards", "", CVAR_ARCHIVE | CVAR_ROM )
UI_CVAR( ui_spVideos, "g_spVideos", "", CVAR_ARCHIVE | CVAR_ROM )
UI_CVAR( ui_spSkill, "g_spSkill", "2", CVAR_ARCHIVE | CVAR_LATCH )

UI_CVAR( ui_spSelection, "ui_spSelection", "", CVAR_ROM )

UI_CVAR( ui_browserMaster, "ui_browserMaster", "0", CVAR_ARCHIVE )
UI_CVAR( ui_browserGameType, "ui_browserGameType", "0", CVAR_ARCHIVE )
UI_CVAR( ui_browserSortKey, "ui_browserSortKey", "4", CVAR_ARCHIVE )
UI_CVAR( ui_browserShowFull, "ui_browserShowFull", "1", CVAR_ARCHIVE )
UI_CVAR( ui_browserShowEmpty, "ui_browserShowEmpty", "1", CVAR_ARCHIVE )

UI_CVAR( ui_brassTime, "cg_brassTime", "2500", CVAR_ARCHIVE )
UI_CVAR( ui_drawCrosshair, "cg_drawCrosshair", "4", CVAR_ARCHIVE )
UI_CVAR( ui_drawCrosshairNames, "cg_drawCrosshairNames", "1", CVAR_ARCHIVE )
UI_CVAR( ui_marks, "cg_marks", "1", CVAR_ARCHIVE )

UI_CVAR( ui_server1, "server1", "", CVAR_ARCHIVE )
UI_CVAR( ui_server2, "server2", "", CVAR_ARCHIVE )
UI_CVAR( ui_server3, "server3", "", CVAR_ARCHIVE )
UI_CVAR( ui_server4, "server4", "", CVAR_ARCHIVE )
UI_CVAR( ui_server5, "server5", "", CVAR_ARCHIVE )
UI_CVAR( ui_server6, "server6", "", CVAR_ARCHIVE )
UI_CVAR( ui_server7, "server7", "", CVAR_ARCHIVE )
UI_CVAR( ui_server8, "server8", "", CVAR_ARCHIVE )
UI_CVAR( ui_server9, "server9", "", CVAR_ARCHIVE )
UI_CVAR( ui_server10, "server10", "", CVAR_ARCHIVE )
UI_CVAR( ui_server11, "server11", "", CVAR_ARCHIVE )
UI_CVAR( ui_server12, "server12", "", CVAR_ARCHIVE )
UI_CVAR( ui_server13, "server13", "", CVAR_ARCHIVE )
UI_CVAR( ui_server14, "server14", "", CVAR_ARCHIVE )
UI_CVAR( ui_server15, "server15", "", CVAR_ARCHIVE )
UI_CVAR( ui_server16, "server16", "", CVAR_ARCHIVE )
UI_CVAR( ui_server17, "server17", "", CVAR_ARCHIVE )
UI_CVAR( ui_server18, "server18", "", CVAR_ARCHIVE )
UI_CVAR( ui_server19, "server19", "", CVAR_ARCHIVE )
UI_CVAR( ui_server20, "server20", "", CVAR_ARCHIVE )
UI_CVAR( ui_server21, "server21", "", CVAR_ARCHIVE )
UI_CVAR( ui_server22, "server22", "", CVAR_ARCHIVE )
UI_CVAR( ui_server23, "server23", "", CVAR_ARCHIVE )
UI_CVAR( ui_server24, "server24", "", CVAR_ARCHIVE )
UI_CVAR( ui_server25, "server25", "", CVAR_ARCHIVE )
UI_CVAR( ui_server26, "server26", "", CVAR_ARCHIVE )
UI_CVAR( ui_server27, "server27", "", CVAR_ARCHIVE )
UI_CVAR( ui_server28, "server28", "", CVAR_ARCHIVE )
UI_CVAR( ui_server29, "server29", "", CVAR_ARCHIVE )
UI_CVAR( ui_server30, "server30", "", CVAR_ARCHIVE )
UI_CVAR( ui_server31, "server31", "", CVAR_ARCHIVE )
UI_CVAR( ui_server32, "server32", "", CVAR_ARCHIVE )

//UI_CVAR( ui_cdkeychecked, "ui_cdkeychecked", "0", CVAR_ROM )

UI_CVAR( ui_trackConsentConfigured, "ui_trackConsentConfigured", "0", CVAR_ARCHIVE )

//new in beta 23:
UI_CVAR( ui_browserOnlyHumans, "ui_browserOnlyHumans", "0", CVAR_ARCHIVE )

//new in beta 37:
UI_CVAR( ui_setupchecked, "ui_setupchecked", "0", CVAR_ARCHIVE )

UI_CVAR( sv_master1, "sv_master1", MASTER_SERVER_NAME, CVAR_ARCHIVE )

UI_CVAR( ui_mapvote_filter, "ui_mapvote_filter", "", 0 )
UI_CVAR( ui_mapvote_sort, "ui_mapvote_sort", "0", CVAR_ARCHIVE )

UI_CVAR( ui_mappage_pagenum, "ui_mappage_pagenum", "0", CVAR_ROM )
UI_CVAR( ui_mappage_page0, "ui_mappage_page0", "", CVAR_ROM )
UI_CVAR( ui_mappage_page1, "ui_mappage_page1", "", CVAR_ROM )
UI_CVAR( ui_mappage_page2, "ui_mappage_page2", "", CVAR_ROM )
UI_CVAR( ui_mappage_page3, "ui_mappage_page3", "", CVAR_ROM )
UI_CVAR( ui_mappage_page4, "ui_mappage_page4", "", CVAR_ROM )
UI_CVAR( ui_mappage_page5, "ui_mappage_page5", "", CVAR_ROM )
UI_CVAR( ui_mappage_page6, "ui_mappage_page6", "", CVAR_ROM )
UI_CVAR( ui_mappage_page7, "ui_mappage_page7", "", CVAR_ROM )
UI_CVAR( ui_mappage_page8, "ui_mappage_page8", "", CVAR_ROM )

UI_CVAR( ui_nextmapvote_remaining, "ui_nextmapvote_remaining", "0", CVAR_ROM )
UI_CVAR( ui_nextmapvote_maps, "ui_nextmapvote_maps", "", CVAR_ROM )
UI_CVAR( ui_nextmapvote_votes, "ui_nextmapvote_votes", "", CVAR_ROM )

#undef UI_CVAR
