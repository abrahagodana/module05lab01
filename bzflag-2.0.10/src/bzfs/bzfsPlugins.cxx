/* bzflag
* Copyright (c) 1993 - 2007 Tim Riker
*
* This package is free software;  you can redistribute it and/or
* modify it under the terms of the license found in the file
* named COPYING that should have accompanied this file.
*
* THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
* IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "common.h"

/* system headers */
#include <string>
#include <vector>
#include <map>

/* common headers */
#include "bzfio.h"
#include "version.h"
#include "TextUtils.h"
#include "commands.h"
#include "bzfsAPI.h"
#include "DirectoryNames.h"


#ifdef _WIN32
std::string extension = ".dll";
std::string globalPluginDir = ".\\plugins\\";
#else
std::string extension = ".so";
std::string globalPluginDir = "$(prefix)/lib/bzflag/";
#endif

typedef std::map<std::string, bz_APIPluginHandler*> tmCustomPluginMap;
tmCustomPluginMap customPluginMap;

typedef struct
{
	std::string plugin;

#ifdef _WIN32
	HINSTANCE	handle;
#else
	void*		handle;
#endif
}trPluginRecord;

std::string findPlugin ( std::string pluginName )
{
	// see if we can just open the bloody thing
	FILE	*fp = fopen(pluginName.c_str(),"rb");
	if (fp)
	{
		fclose(fp);
		return pluginName;
	}

	// now try it with the standard extension
	std::string name = pluginName + extension;
	fp = fopen(name.c_str(),"rb");
	if (fp)
	{
		fclose(fp);
		return name;
	}

	// check the local users plugins dir
	name = getConfigDirName(BZ_CONFIG_DIR_VERSION) + pluginName + extension;
	fp = fopen(name.c_str(),"rb");
	if (fp)
	{
		fclose(fp);
		return name;
	}

	// check the global plugins dir
	name = globalPluginDir + pluginName + extension;
	fp = fopen(name.c_str(),"rb");
	if (fp)
	{
		fclose(fp);
		return name;
	}

	return std::string("");
}

std::vector<trPluginRecord>	vPluginList;

void unload1Plugin ( int iPluginID );

#ifdef _WIN32
#  include <windows.h>

int getPluginVersion ( HINSTANCE hLib )
{
	int (*lpProc)(void);
	lpProc = (int (__cdecl *)(void))GetProcAddress(hLib, "bz_GetVersion");
	if (lpProc)
		return lpProc();
	return 0;
}

bool load1Plugin ( std::string plugin, std::string config )
{
	int (*lpProc)(const char*);

	std::string realPluginName = findPlugin(plugin);

	HINSTANCE	hLib = LoadLibrary(realPluginName.c_str());
	if (hLib)
	{
		if (getPluginVersion(hLib) < BZ_API_VERSION)
		{
			logDebugMessage(1,"Plugin:%s found but expects an older API version (%d), upgrade it\n",plugin.c_str(),getPluginVersion(hLib));
			FreeLibrary(hLib);
			return false;
		}
		else
		{
			lpProc = (int (__cdecl *)(const char*))GetProcAddress(hLib, "bz_Load");
			if (lpProc)
			{
				lpProc(config.c_str());
				logDebugMessage(1,"Plugin:%s loaded\n",plugin.c_str());

				trPluginRecord pluginRecord;
				pluginRecord.handle = hLib;
				pluginRecord.plugin = plugin;
				vPluginList.push_back(pluginRecord);
				return true;
			}
			else
			{
				logDebugMessage(1,"Plugin:%s found but does not contain bz_Load method\n",plugin.c_str());
				FreeLibrary(hLib);
				return false;
			}
		}
	}
	else
	{
		logDebugMessage(1,"Plugin:%s not found\n",plugin.c_str());
		return false;
	}
}

void unload1Plugin ( int iPluginID )
{
	int (*lpProc)(void);

	trPluginRecord &plugin = vPluginList[iPluginID];

	lpProc = (int (__cdecl *)(void))GetProcAddress(plugin.handle, "bz_Unload");
	if (lpProc)
		lpProc();
	else
		logDebugMessage(1,"Plugin does not contain bz_UnLoad method\n");

	FreeLibrary(plugin.handle);
	plugin.handle = NULL;
}
#else

#  include <dlfcn.h>
std::vector<void*>	vLibHandles;

int getPluginVersion ( void* hLib )
{
	int (*lpProc)(void);
	*(void**) &lpProc = dlsym(hLib,"bz_GetVersion");
	if (lpProc)
		return (*lpProc)();
	return 0;
}

bool load1Plugin ( std::string plugin, std::string config )
{
	int (*lpProc)(const char*);

	std::string realPluginName = findPlugin(plugin);

	void *hLib = dlopen(realPluginName.c_str(), RTLD_LAZY | RTLD_GLOBAL);
	if (hLib)
	{
		if (dlsym(hLib, "bz_Load") == NULL) {
			logDebugMessage(1,"Plugin:%s found but does not contain bz_Load method, error %s\n",plugin.c_str(),dlerror());
			dlclose(hLib);
			return false;
		}

		int version = getPluginVersion(hLib);
		if (version < BZ_API_VERSION)
		{
			logDebugMessage(1,"Plugin:%s found but expects an older API version (%d), upgrade it\n", plugin.c_str(), version);
			dlclose(hLib);
			return false;
		}
		else
		{
			*(void**) &lpProc = dlsym(hLib,"bz_Load");
			if (lpProc)
			{
				(*lpProc)(config.c_str());
				logDebugMessage(1,"Plugin:%s loaded\n",plugin.c_str());
				trPluginRecord pluginRecord;
				pluginRecord.handle = hLib;
				pluginRecord.plugin = plugin;
				vPluginList.push_back(pluginRecord);
				return true;
			}
		}
	}
	else
	{
		logDebugMessage(1,"Plugin:%s not found, error %s\n",plugin.c_str(), dlerror());
		return false;
	}

	logDebugMessage(1,"If you see this, there is something terribly wrong.\n");
	return false;
}

void unload1Plugin ( int iPluginID )
{
	int (*lpProc)(void);
	trPluginRecord &plugin = vPluginList[iPluginID];

	*(void**) &lpProc = dlsym(plugin.handle, "bz_Unload");
	if (lpProc)
		(*lpProc)();
	else
		logDebugMessage(1,"Plugin does not contain bz_UnLoad method, error %s\n",dlerror());

	dlclose(plugin.handle);
	plugin.handle = NULL;
}
#endif


bool loadPlugin ( std::string plugin, std::string config )
{
	// check and see if it's an extension we have a handler for
	std::string ext;

	std::vector<std::string> parts = TextUtils::tokenize(plugin,std::string("."));
	ext = parts[parts.size()-1];

	tmCustomPluginMap::iterator itr = customPluginMap.find(TextUtils::tolower(ext));

	if (itr != customPluginMap.end() && itr->second)
	{
		bz_APIPluginHandler *handler = itr->second;
		return handler->handle(plugin,config);
	}
	else
		return load1Plugin(plugin,config);
}

bool unloadPlugin ( std::string plugin )
{
	// unload the first one of the name we find
	for (unsigned int i = 0; i < vPluginList.size();i++)
	{
		if ( vPluginList[i].plugin == plugin )
		{
			unload1Plugin(i);
			vPluginList.erase(vPluginList.begin()+i);
			return true;
		}
	}
	return false;
}

void unloadPlugins ( void )
{
	for (unsigned int i = 0; i < vPluginList.size();i++)
		unload1Plugin(i);
	vPluginList.clear();

	removeCustomSlashCommand("loadplugin");
	removeCustomSlashCommand("unloadplugin");
	removeCustomSlashCommand("listplugins");
}

std::vector<std::string> getPluginList ( void )
{
	std::vector<std::string> plugins;
	for (unsigned int i = 0; i < vPluginList.size();i++)
		plugins.push_back(vPluginList[i].plugin);

	return plugins;
}

void parseServerCommand(const char *message, int dstPlayerId);

class DynamicPluginCommands : public bz_CustomSlashCommandHandler
{
public:
	virtual ~DynamicPluginCommands(){};
	virtual bool handle ( int playerID, bzApiString _command, bzApiString _message, bzAPIStringList *params )
	{
		bz_PlayerRecord	record;

		std::string command = _command.c_str();
		std::string message = _message.c_str();

		bz_PlayerRecord	*p = bz_getPlayerByIndex(playerID);
		if (!p)
			return false;

		record = *p;

		bz_freePlayerRecord(p);

		// list dosn't need admin
		if ( TextUtils::tolower(command) == "listplugins" )
		{
			std::vector<std::string>	plugins = getPluginList();

			if (!plugins.size())
				bz_sendTextMessage(BZ_SERVER,playerID,"No Plug-ins loaded.");
			else
			{
				bz_sendTextMessage(BZ_SERVER,playerID,"Plug-ins loaded:");

				for ( unsigned int i = 0; i < plugins.size(); i++)
					bz_sendTextMessage(BZ_SERVER,playerID,plugins[i].c_str());
			}
			return true;
		}

		if (!record.hasPerm("PLUGINS"))
		{
			bz_sendTextMessage(BZ_SERVER,playerID,"You do not have permission to (un)load plug-ins.");
			return true;
		}

		if ( TextUtils::tolower(command) == "loadplugin" )
		{
			if ( !params->size() )
			{
				bz_sendTextMessage(BZ_SERVER,playerID,"Usage: /loadplugin plug-in");
				return true;
			}

			std::vector<std::string> subparams = TextUtils::tokenize(message,std::string(","));

			std::string config;
			if ( subparams.size() >1)
				config = subparams[1];

			if (loadPlugin(subparams[0],config))
				bz_sendTextMessage(BZ_SERVER,playerID,"Plug-in loaded.");
			else
				bz_sendTextMessage(BZ_SERVER,playerID,"Plug-in load failed.");
			return true;
		}

		if ( TextUtils::tolower(command) == "unloadplugin" )
		{
			if ( !params->size() )
			{
				bz_sendTextMessage(BZ_SERVER,playerID,"Usage: /unloadplugin plug-in");
				return true;
			}

			if ( unloadPlugin(std::string(params->get(0).c_str())) )
				bz_sendTextMessage(BZ_SERVER,playerID,"Plug-in unloaded.");

			return true;
		}
		return true;
	}
};

DynamicPluginCommands	command;

void initPlugins ( void )
{
	customPluginMap.clear();

	registerCustomSlashCommand("loadplugin",&command);
	registerCustomSlashCommand("unloadplugin",&command);
	registerCustomSlashCommand("listplugins",&command);
}

bool registerCustomPluginHandler ( std::string exte, bz_APIPluginHandler *handler )
{
	std::string ext = TextUtils::tolower(exte);
	customPluginMap[ext] = handler;
	return true;
}

bool removeCustomPluginHandler ( std::string ext, bz_APIPluginHandler *handler )
{
	tmCustomPluginMap::iterator itr = customPluginMap.find(TextUtils::tolower(ext));

	if (itr == customPluginMap.end() || itr->second != handler)
		return false;

	customPluginMap.erase(itr);
	return true;
}


// Local Variables: ***
// mode:C++ ***
// tab-width: 8 ***
// c-basic-offset: 2 ***
// indent-tabs-mode: t ***
// End: ***
// ex: shiftwidth=2 tabstop=8
