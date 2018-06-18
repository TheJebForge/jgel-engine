#ifndef SVFUNCTIONS_H_INCLUDED
#define SVFUNCTIONS_H_INCLUDED

int checkArgs(lua_State *L, vector<string> types){
    if(lua_gettop(L)==types.size()){
        bool out = true;
        for(int i=1;i<=lua_gettop(L);i++){
            string fg = lua_typename(L,lua_type(L,i));
            if(fg!=types[i-1]){
                out = false;
            }
        }
        if(out)
            return 1;
        else
            return 0;
    } else {
        return -1;
    }
}

int checkArgsUnDef(lua_State *L, vector<string> types){
        bool out = true;
        for(int i=1;i<=types.size();i++){
            string fg = lua_typename(L,lua_type(L,i));
            if(fg!=types[i-1]){
                out = false;
            }
        }
        if(out)
            return 1;
        else
            return 0;
}

int svOpenSlot(lua_State *L){
    vector<string> args {"number"};
    if(checkArgs(L,args)==1){
        lua_pushinteger(L,sv_open_slot(lua_tointeger(L,1)));
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}

int svCloseSlot(lua_State *L){
    vector<string> args {"number"};
    if(checkArgs(L,args)==1){
        lua_pushinteger(L,sv_close_slot(lua_tointeger(L,1)));
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}

int svLockSlot(lua_State *L){
    vector<string> args {"number"};
    if(checkArgs(L,args)==1){
        lua_pushinteger(L,sv_lock_slot(lua_tointeger(L,1)));
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}

int svUnlockSlot(lua_State *L){
    vector<string> args {"number"};
    if(checkArgs(L,args)==1){
        lua_pushinteger(L,sv_unlock_slot(lua_tointeger(L,1)));
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}

int svGetSampleType(lua_State *L){
    vector<string> args {};
    if(checkArgs(L,args)==1){
        lua_pushinteger(L,sv_get_sample_type());
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}

int svLoad(lua_State *L){
    vector<string> args {"number","string"};
    if(checkArgs(L,args)==1){
        lua_pushinteger(L,sv_load(lua_tointeger(L,1),lua_tostring(L,2)));
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}
int svPlay(lua_State *L){
    vector<string> args {"number"};
    if(checkArgs(L,args)==1){
        lua_pushinteger(L,sv_play(lua_tointeger(L,1)));
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}

int svPlayFromBeginning(lua_State *L){
    vector<string> args {"number"};
    if(checkArgs(L,args)==1){
        lua_pushinteger(L,sv_play_from_beginning(lua_tointeger(L,1)));
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}
int svStop(lua_State *L){
    vector<string> args {"number"};
    if(checkArgs(L,args)==1){
        lua_pushinteger(L,sv_stop(lua_tointeger(L,1)));
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}

int svSetAutostop(lua_State *L){
    vector<string> args {"number","number"};
    if(checkArgs(L,args)==1){
        lua_pushinteger(L,sv_set_autostop(lua_tointeger(L,1),lua_tointeger(L,2)));
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}

int svEndOfSong(lua_State *L){
    vector<string> args {"number"};
    if(checkArgs(L,args)==1){
        lua_pushinteger(L,sv_end_of_song(lua_tointeger(L,1)));
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}

int svRewind(lua_State *L){
    vector<string> args {"number","number"};
    if(checkArgs(L,args)==1){
        lua_pushinteger(L,sv_rewind(lua_tointeger(L,1),lua_tointeger(L,2)));
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}

int svVolume(lua_State *L){
    vector<string> args {"number","number"};
    if(checkArgs(L,args)==1){
        lua_pushinteger(L,sv_volume(lua_tointeger(L,1),lua_tointeger(L,2)));
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}

int svSendEvent(lua_State *L){
    vector<string> args {"number","number","number","number","number","number","number"};
    if(checkArgs(L,args)==1){
        lua_pushinteger(L,sv_send_event(lua_tointeger(L,1),lua_tointeger(L,2),lua_tointeger(L,3),lua_tointeger(L,4),lua_tointeger(L,5),lua_tointeger(L,6),lua_tointeger(L,7)));
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}

int svGetCurrentLine(lua_State *L){
    vector<string> args {"number"};
    if(checkArgs(L,args)==1){
        lua_pushinteger(L,sv_get_current_line(lua_tointeger(L,1)));
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}

int svGetCurrentLine2(lua_State *L){
    vector<string> args {"number"};
    if(checkArgs(L,args)==1){
        lua_pushinteger(L,sv_get_current_line2(lua_tointeger(L,1)));
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}

int svGetCurrentSignalLevel(lua_State *L){
    vector<string> args {"number","number"};
    if(checkArgs(L,args)==1){
        lua_pushinteger(L,sv_get_current_signal_level(lua_tointeger(L,1),lua_tointeger(L,2)));
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}

int svGetSongName(lua_State *L){
    vector<string> args {"number"};
    if(checkArgs(L,args)==1){
        lua_pushstring(L,sv_get_song_name(lua_tointeger(L,1)));
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}

int svGetSongBPM(lua_State *L){
    vector<string> args {"number"};
    if(checkArgs(L,args)==1){
        lua_pushinteger(L,sv_get_song_bpm(lua_tointeger(L,1)));
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}

int svGetSongTPL(lua_State *L){
    vector<string> args {"number"};
    if(checkArgs(L,args)==1){
        lua_pushinteger(L,sv_get_song_tpl(lua_tointeger(L,1)));
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}

int svGetSongLengthFrames(lua_State *L){
    vector<string> args {"number"};
    if(checkArgs(L,args)==1){
        lua_pushinteger(L,sv_get_song_length_frames(lua_tointeger(L,1)));
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}

int svGetSongLengthLines(lua_State *L){
    vector<string> args {"number"};
    if(checkArgs(L,args)==1){
        lua_pushinteger(L,sv_get_song_length_lines(lua_tointeger(L,1)));
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}

int svNewModule(lua_State *L){
    vector<string> args {"number","string", "string","number","number","number"};
    if(checkArgs(L,args)==1){
        lua_pushinteger(L,sv_new_module(lua_tointeger(L,1),lua_tostring(L,2),lua_tostring(L,3),lua_tointeger(L,4),lua_tointeger(L,5),lua_tointeger(L,6)));
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}

int svRemoveModule(lua_State *L){
    vector<string> args {"number","number"};
    if(checkArgs(L,args)==1){
        lua_pushinteger(L,sv_remove_module(lua_tointeger(L,1),lua_tointeger(L,2)));
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}

int svConnectModule(lua_State *L){
    vector<string> args {"number","number","number"};
    if(checkArgs(L,args)==1){
        lua_pushinteger(L,sv_connect_module(lua_tointeger(L,1),lua_tointeger(L,2),lua_tointeger(L,3)));
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}

int svDisconnectModule(lua_State *L){
    vector<string> args {"number","number","number"};
    if(checkArgs(L,args)==1){
        lua_pushinteger(L,sv_disconnect_module(lua_tointeger(L,1),lua_tointeger(L,2),lua_tointeger(L,3)));
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}

int svLoadModule(lua_State *L){
    vector<string> args {"number","string","number","number","number"};
    if(checkArgs(L,args)==1){
        lua_pushinteger(L,sv_load_module(lua_tointeger(L,1),lua_tostring(L,2),lua_tointeger(L,3),lua_tointeger(L,4),lua_tointeger(L,5)));
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}

int svSamplerLoad(lua_State *L){
    vector<string> args {"number","number","string","number"};
    if(checkArgs(L,args)==1){
        lua_pushinteger(L,sv_sampler_load(lua_tointeger(L,1),lua_tointeger(L,2),lua_tostring(L,3),lua_tointeger(L,4)));
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}

int svGetNumberOfModules(lua_State *L){
    vector<string> args {"number"};
    if(checkArgs(L,args)==1){
        lua_pushinteger(L,sv_get_number_of_modules(lua_tointeger(L,1)));
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}

int svGetModuleFlags(lua_State *L){
    vector<string> args {"number","number"};
    if(checkArgs(L,args)==1){
        lua_pushinteger(L,sv_get_module_flags(lua_tointeger(L,1),lua_tointeger(L,2)));
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}

int svGetModuleInputs(lua_State *L){
    vector<string> args {"number","number"};
    if(checkArgs(L,args)==1){
        lua_newtable(L);
        int* a = sv_get_module_inputs(lua_tointeger(L,1),lua_tointeger(L,2));
        for(int i = 0;i<sizeof(a)/sizeof(a[0]);i++){
            lua_pushinteger(L,a[i]);
            lua_settable(L,lua_gettop(L)-1);
        }
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}

int svGetModuleOutputs(lua_State *L){
    vector<string> args {"number","number"};
    if(checkArgs(L,args)==1){
        lua_newtable(L);
        int* a = sv_get_module_outputs(lua_tointeger(L,1),lua_tointeger(L,2));
        for(int i = 0;i<sizeof(a)/sizeof(a[0]);i++){
            lua_pushinteger(L,a[i]);
            lua_settable(L,lua_gettop(L)-1);
        }
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}

int svGetModuleName(lua_State *L){
    vector<string> args {"number","number"};
    if(checkArgs(L,args)==1){
        lua_pushstring(L,sv_get_module_name(lua_tointeger(L,1),lua_tointeger(L,2)));
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}

int svGetModuleXY(lua_State *L){
    vector<string> args {"number","number"};
    if(checkArgs(L,args)==1){
        lua_pushinteger(L,sv_get_module_xy(lua_tointeger(L,1),lua_tointeger(L,2)));
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}

int svGetModuleColor(lua_State *L){
    vector<string> args {"number","number"};
    if(checkArgs(L,args)==1){
        lua_pushinteger(L,sv_get_module_color(lua_tointeger(L,1),lua_tointeger(L,2)));
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}

int svGetNumberOfModuleCtls(lua_State *L){
    vector<string> args {"number","number"};
    if(checkArgs(L,args)==1){
        lua_pushinteger(L,sv_get_number_of_module_ctls(lua_tointeger(L,1),lua_tointeger(L,2)));
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}

int svGetModuleCtlName(lua_State *L){
    vector<string> args {"number","number","number"};
    if(checkArgs(L,args)==1){
        lua_pushstring(L,sv_get_module_ctl_name(lua_tointeger(L,1),lua_tointeger(L,2),lua_tointeger(L,3)));
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}

int svGetModuleCtlValue(lua_State *L){
    vector<string> args {"number","number","number","number"};
    if(checkArgs(L,args)==1){
        lua_pushinteger(L,sv_get_module_ctl_value(lua_tointeger(L,1),lua_tointeger(L,2),lua_tointeger(L,3),lua_tointeger(L,4)));
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}

int svGetNumberOfPatterns(lua_State *L){
    vector<string> args {"number"};
    if(checkArgs(L,args)==1){
        lua_pushinteger(L,sv_get_number_of_patterns(lua_tointeger(L,1)));
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}

int svGetPatternX(lua_State *L){
    vector<string> args {"number","number"};
    if(checkArgs(L,args)==1){
        lua_pushinteger(L,sv_get_pattern_x(lua_tointeger(L,1),lua_tointeger(L,2)));
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}

int svGetPatternY(lua_State *L){
    vector<string> args {"number","number"};
    if(checkArgs(L,args)==1){
        lua_pushinteger(L,sv_get_pattern_y(lua_tointeger(L,1),lua_tointeger(L,2)));
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}

int svGetPatternTracks(lua_State *L){
    vector<string> args {"number","number"};
    if(checkArgs(L,args)==1){
        lua_pushinteger(L,sv_get_pattern_tracks(lua_tointeger(L,1),lua_tointeger(L,2)));
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}

int svGetPatternLines(lua_State *L){
    vector<string> args {"number","number"};
    if(checkArgs(L,args)==1){
        lua_pushinteger(L,sv_get_pattern_lines(lua_tointeger(L,1),lua_tointeger(L,2)));
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}

int svPatternMute(lua_State *L){
    vector<string> args {"number","number","number"};
    if(checkArgs(L,args)==1){
        lua_pushinteger(L,sv_pattern_mute(lua_tointeger(L,1),lua_tointeger(L,2),lua_tointeger(L,3)));
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}

int svGetTicks(lua_State *L){
    vector<string> args {};
    if(checkArgs(L,args)==1){
        lua_pushinteger(L,sv_get_ticks());
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}

int svGetTicksPerSecond(lua_State *L){
    vector<string> args {};
    if(checkArgs(L,args)==1){
        lua_pushinteger(L,sv_get_ticks_per_second());
        return 1;
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Wrong args");
        return 2;
    }
}

#endif // SVFUNCTIONS_H_INCLUDED
