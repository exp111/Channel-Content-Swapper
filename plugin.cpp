/*
 * TeamSpeak 3 demo plugin
 *
 * Copyright (c) 2008-2017 TeamSpeak Systems GmbH
 */

#ifdef _WIN32
#pragma warning (disable : 4100)  /* Disable Unreferenced parameter warning */
#include <Windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "teamspeak/public_errors.h"
#include "teamspeak/public_errors_rare.h"
#include "teamspeak/public_definitions.h"
#include "teamspeak/public_rare_definitions.h"
#include "teamspeak/clientlib_publicdefinitions.h"
#include "ts3_functions.h"
#include "plugin.h"
#include <iostream>
#include <string>

static struct TS3Functions ts3Functions;

#ifdef _WIN32
#define _strcpy(dest, destSize, src) strcpy_s(dest, destSize, src)
#define snprintf sprintf_s
#else
#define _strcpy(dest, destSize, src) { strncpy(dest, src, destSize-1); (dest)[destSize-1] = '\0'; }
#endif

#define PLUGIN_API_VERSION 22

#define PATH_BUFSIZE 512
#define COMMAND_BUFSIZE 128
#define INFODATA_BUFSIZE 128
#define SERVERINFO_BUFSIZE 256
#define CHANNELINFO_BUFSIZE 512
#define RETURNCODE_BUFSIZE 128

static char* pluginID = NULL;

#ifdef _WIN32
/* Helper function to convert wchar_T to Utf-8 encoded strings on Windows */
static int wcharToUtf8(const wchar_t* str, char** result) {
	int outlen = WideCharToMultiByte(CP_UTF8, 0, str, -1, 0, 0, 0, 0);
	*result = (char*)malloc(outlen);
	if(WideCharToMultiByte(CP_UTF8, 0, str, -1, *result, outlen, 0, 0) == 0) {
		*result = NULL;
		return -1;
	}
	return 0;
}
#endif

/*********************************** Required functions ************************************/


const char* ts3plugin_name() {
#ifdef _WIN32
	static char* result = NULL;  /* Static variable so it's allocated only once */
	if(!result) {
		const wchar_t* name = L"Channel Content Swapper";
		if(wcharToUtf8(name, &result) == -1) {  /* Convert name into UTF-8 encoded result */
			result = "Channel Content Swapper";  /* Conversion failed, fallback here */
		}
	}
	return result;
#else
	return "Channel Content Swapper";
#endif
}

const char* ts3plugin_version() {
    return "1.0";
}

int ts3plugin_apiVersion() {
	return PLUGIN_API_VERSION;
}

const char* ts3plugin_author() {
    return "Exp";
}

const char* ts3plugin_description() {
    return "Right-click to swap the users of two channels (either your current or the set target channel). Yes, people are content.";
}

void ts3plugin_setFunctionPointers(const struct TS3Functions funcs) {
    ts3Functions = funcs;
}

int ts3plugin_init() {
    char appPath[PATH_BUFSIZE];
    char resourcesPath[PATH_BUFSIZE];
    char configPath[PATH_BUFSIZE];
	char pluginPath[PATH_BUFSIZE];

    ts3Functions.getAppPath(appPath, PATH_BUFSIZE);
    ts3Functions.getResourcesPath(resourcesPath, PATH_BUFSIZE);
    ts3Functions.getConfigPath(configPath, PATH_BUFSIZE);
	ts3Functions.getPluginPath(pluginPath, PATH_BUFSIZE, pluginID);

    return 0; 
}

void ts3plugin_shutdown() {
	if(pluginID) {
		free(pluginID);
		pluginID = NULL;
	}
}

int ts3plugin_offersConfigure() {
	return PLUGIN_OFFERS_NO_CONFIGURE;
}

void ts3plugin_registerPluginID(const char* id) {
	const size_t sz = strlen(id) + 1;
	pluginID = (char*)malloc(sz * sizeof(char));
	_strcpy(pluginID, sz, id);
}

const char* ts3plugin_commandKeyword() {
	return "";
}

const char* ts3plugin_infoTitle() {
	return "Channel Content Swapper";
}

void ts3plugin_freeMemory(void* data) {
	free(data);
}
int ts3plugin_requestAutoload() {
	return 0;
}

//Global Variables
uint64 targetChannel = 0;

//Helper Functions
bool isClientInList(anyID* clientList, anyID* clientID) {
	for (int i = 0; clientList[i]; i++)
	{
		if (clientList[i] == *clientID) return true;
	}

	return false;
}

void HackDavid(int two, int one)
{
	//2 + 2 is 4 minus 1 that's 3 -> quick math
	int four = two + two;
	int three = four - one;
}

void swapContents(uint64 serverConnectionHandlerID, uint64 firstChannel, uint64 secondChannel)
{
	anyID* firstClientList;
	ts3Functions.getChannelClientList(serverConnectionHandlerID, firstChannel, &firstClientList);

	anyID* secondClientList;
	ts3Functions.getChannelClientList(serverConnectionHandlerID, secondChannel, &secondClientList);

	anyID* clientList;
	ts3Functions.getClientList(serverConnectionHandlerID, &clientList);

	int clientType;
	for (int i = 0;  clientList[i]; i++)
	{
		if (ts3Functions.getClientVariableAsInt(serverConnectionHandlerID, clientList[i], CLIENT_TYPE, &clientType) != ERROR_ok)
			continue;

		if (clientType == 1)
			continue;

		if (!isClientInList(firstClientList, &clientList[i]))
			continue;

		ts3Functions.requestClientMove(serverConnectionHandlerID, clientList[i], secondChannel, "", "");
	}
	
	for (int i = 0; clientList[i]; i++)
	{
		if (ts3Functions.getClientVariableAsInt(serverConnectionHandlerID, clientList[i], CLIENT_TYPE, &clientType) != ERROR_ok)
			continue;

		if (clientType == 1)
			continue;

		if (!isClientInList(secondClientList, &clientList[i]))
			continue;

		ts3Functions.requestClientMove(serverConnectionHandlerID, clientList[i], firstChannel, "", "");
	}
}

/* Helper function to create a menu item */
static struct PluginMenuItem* createMenuItem(enum PluginMenuType type, int id, const char* text, const char* icon) {
	struct PluginMenuItem* menuItem = (struct PluginMenuItem*)malloc(sizeof(struct PluginMenuItem));
	menuItem->type = type;
	menuItem->id = id;
	_strcpy(menuItem->text, PLUGIN_MENU_BUFSZ, text);
	_strcpy(menuItem->icon, PLUGIN_MENU_BUFSZ, icon);
	return menuItem;
}

/* Some makros to make the code to create menu items a bit more readable */
#define BEGIN_CREATE_MENUS(x) const size_t sz = x + 1; size_t n = 0; *menuItems = (struct PluginMenuItem**)malloc(sizeof(struct PluginMenuItem*) * sz);
#define CREATE_MENU_ITEM(a, b, c, d) (*menuItems)[n++] = createMenuItem(a, b, c, d);
#define END_CREATE_MENUS (*menuItems)[n++] = NULL; assert(n == sz);


enum {
	MENU_ID_CHANNEL_1 = 1,
	MENU_ID_CHANNEL_2,
	MENU_ID_CHANNEL_3
};

void ts3plugin_initMenus(struct PluginMenuItem*** menuItems, char** menuIcon) {

	BEGIN_CREATE_MENUS(3);
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CHANNEL, MENU_ID_CHANNEL_1, "Swap Channel Content with current Channel", "");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CHANNEL, MENU_ID_CHANNEL_2, "Set Target Channel", "");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CHANNEL, MENU_ID_CHANNEL_3, "Swap Channel Content with Target Channel", "");
	END_CREATE_MENUS; 

	*menuIcon = (char*)malloc(PLUGIN_MENU_BUFSZ * sizeof(char));
	_strcpy(*menuIcon, PLUGIN_MENU_BUFSZ, ""); //PLUGIN MENU IMAGE
}

/************************** TeamSpeak callbacks ***************************/


void ts3plugin_onMenuItemEvent(uint64 serverConnectionHandlerID, enum PluginMenuType type, int menuItemID, uint64 selectedItemID) {
	switch(type) {
		case PLUGIN_MENU_TYPE_CHANNEL:
			/* Channel contextmenu item was triggered. selectedItemID is the channelID of the selected channel */
			switch(menuItemID) {
				case MENU_ID_CHANNEL_1:
					/* Swap Channel Content with current Channel */
					anyID mClientID;
					ts3Functions.getClientID(serverConnectionHandlerID, &mClientID);
					uint64 currentChannel;
					ts3Functions.getChannelOfClient(serverConnectionHandlerID, mClientID, &currentChannel);
					//Swap!
					swapContents(serverConnectionHandlerID, currentChannel, selectedItemID);
					break;
				case MENU_ID_CHANNEL_2:
					/* Set Target Channel */
					targetChannel = selectedItemID;
					break;
				case MENU_ID_CHANNEL_3:
					/* Swap Channel Content with Target Channel */
					if (targetChannel == 0)
					{
						//We don't have a target channel
						ts3Functions.printMessageToCurrentTab("Select a Target Channel first");
					}
					else {
						//Swap!
						swapContents(serverConnectionHandlerID, selectedItemID, targetChannel);
					}
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}
}

void ts3plugin_onConnectStatusChangeEvent(uint64 serverConnectionHandlerID, int newStatus, unsigned int errorNumber)
{
	if (newStatus == STATUS_CONNECTION_ESTABLISHED) //On new connection reset targetchannel
		targetChannel = 0;
}

const char* ts3plugin_keyDeviceName(const char* keyIdentifier) {
	return NULL;
}

const char* ts3plugin_displayKeyText(const char* keyIdentifier) {
	return NULL;
}

const char* ts3plugin_keyPrefix() {
	return NULL;
}
