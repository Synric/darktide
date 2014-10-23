#include "Noggit.h"

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#include <winerror.h>
#endif

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <list>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/thread/thread.hpp>

#include <SDL.h>

#include "AppState.h"
#include "AsyncLoader.h"
#include "ConfigFile.h"
#include "Environment.h"  // This singleton holds all vars you dont must save. Like bools for display options. We should move all global stuff here to get it OOP!
#include "errorHandling.h"
#include "Liquid.h"
#include "Log.h"
#include "MapView.h"
#include "Menu.h"
#include "Model.h"
#include "MPQ.h"
#include "Project.h"    // This singleton holds later all settings for the current project. Will also be serialized to a selectable place on disk.
#include "revision.h"
#include "Settings.h"    // In this singleton you can insert user settings. This object will later be serialized to disk (userpath)
#include "Video.h"
#include "TextureManager.h" // TextureManager::report()
#include "WMO.h" // WMOManager::report()
#include "ModelManager.h" // ModelManager::report()

Noggit app;
void CreateStrips();
extern std::list<std::string> gListfile;

Noggit::Noggit()
  : fullscreen(false)
  , doAntiAliasing(true)
  , xres(1280)
  , yres(720)
{}

Noggit::~Noggit()
{

}

void Noggit::initPath(char *argv[])
{
  try
  {
    boost::filesystem::path startupPath( argv[0] );
    startupPath.remove_filename();

    if( startupPath.is_relative() )
    {
      boost::filesystem::current_path( boost::filesystem::current_path() / startupPath );
    }
    else
    {
      boost::filesystem::current_path( startupPath );
    }
  }
  catch( const boost::filesystem::filesystem_error& ex )
  {
    LogError << ex.what() << std::endl;
  }
}

void Noggit::initFont()
{

  std::string arialFilename( "<PLEASE GET SOME FONT FOR YOUR OS>" );
#ifdef _WIN32
  //! \todo This might not work on windows 7 or something. Please fix.
  arialFilename = "C:\\windows\\fonts\\arial.ttf";
#endif
#ifdef __APPLE__
  arialFilename = "/Library/Fonts/Arial.ttf";
#endif
  if( !boost::filesystem::exists( arialFilename ) )
  {
    arialFilename = "arial.ttf";
    if( !boost::filesystem::exists( arialFilename ) )
    {
      arialFilename = "fonts/arial.ttf";
      if( !boost::filesystem::exists( arialFilename ) )
      {
        LogError << "Can not find arial.ttf." << std::endl;
        //return -1;
      }
    }
  }

  // Initializing Fonts
  skurri32.init( "fonts/skurri.ttf", 32, true );
  fritz16.init( "fonts/frizqt__.ttf", 16, true );
  morpheus40.init( "fonts/morpheus.ttf", 40, true );
  arialn13.init( "fonts/arialn.ttf", 13, true );

  arial12.init( arialFilename, 12, false );
  arial14.init( arialFilename, 14, false );
  arial16.init( arialFilename, 16, false );
  arial24.init( arialFilename, 24, false );
  arial32.init( arialFilename, 32, false );

}

void Noggit::initEnv()
{
  // init
  Environment::getInstance()->cursorColorR = 1.0f;
  Environment::getInstance()->cursorColorG = 1.0f;
  Environment::getInstance()->cursorColorB = 1.0f;
  Environment::getInstance()->cursorColorA = 1.0f;
  Environment::getInstance()->cursorType = 1;

  // load cursor settings
  if(boost::filesystem::exists("noggit.conf"))
  {
    ConfigFile myConfigfile = ConfigFile( "noggit.conf" );
    if( myConfigfile.keyExists("RedColor") && myConfigfile.keyExists("GreenColor")  &&  myConfigfile.keyExists("BlueColor") &&  myConfigfile.keyExists("AlphaColor") )
    {
      Environment::getInstance()->cursorColorR = myConfigfile.read<float>( "RedColor" );
      Environment::getInstance()->cursorColorG = myConfigfile.read<float>( "GreenColor" );
      Environment::getInstance()->cursorColorB = myConfigfile.read<float>( "BlueColor" );
      Environment::getInstance()->cursorColorA = myConfigfile.read<float>( "AlphaColor" );
    }

    if( myConfigfile.keyExists("CursorType"))
      Environment::getInstance()->cursorType = myConfigfile.read<int>( "CursorType" );
  }

  Settings::getInstance();
  Project::getInstance();
  Environment::getInstance();
}

void Noggit::parseArgs(int argc, char *argv[])
{
  // handle starting parameters
  for( int i( 1 ); i < argc; ++i )
  {
    if( !strcmp( argv[i], "-f" ) || !strcmp( argv[i], "-fullscreen" ) )
    {
      fullscreen = true;
    }
    else if( !strcmp( argv[i], "-na" ) || !strcmp( argv[i], "-noAntiAliasing" ) )
    {
      doAntiAliasing = false;
    }
    else if (!strcmp(argv[i],"-1024") || !strcmp(argv[i],"-1024x768")) {
      xres = 1024;
      yres = 768;
    }
    else if (!strcmp(argv[i],"-800") || !strcmp(argv[i],"-800x600")) {
      xres = 800;
      yres = 600;
    }
    else if (!strcmp(argv[i],"-1280") || !strcmp(argv[i],"-1280x1024")) {
      xres = 1280;
      yres = 1024;
    }
    else if (!strcmp(argv[i],"-1280x960")) {
      xres = 1280;
      yres = 960;
    }
    else if (!strcmp(argv[i],"-1280x720")) {
      xres = 1280;
      yres = 720;
    }
    else if (!strcmp(argv[i],"-1400") || !strcmp(argv[i],"-1400x1050")) {
      xres = 1400;
      yres = 1050;
    }
    else if (!strcmp(argv[i],"-1280x800")) {
      xres = 1280;
      yres = 800;
    }
    else if (!strcmp(argv[i],"-1600") || !strcmp(argv[i],"-1600x1200")) {
      xres = 1600;
      yres = 1200;
    }
    else if (!strcmp(argv[i],"-1920") || !strcmp(argv[i],"-1920x1200")) {
      xres = 1920;
      yres = 1200;
    }
    else if (!strcmp(argv[i],"-2048") || !strcmp(argv[i],"-2048x1536")) {
      xres = 2048;
      yres = 1536;
    }
  }

  if( Settings::getInstance()->noAntiAliasing() )
  {
    doAntiAliasing = false;
  }
}

std::string Noggit::getGamePath()
{
  if( !boost::filesystem::exists( "noggit.conf" ) )
  {
    Log << "DON NOT find a config file." << std::endl;

    if( boost::filesystem::exists( "noggit.conf.conf" ) )
    {
      Log << "Error: You have named your config file noggit.conf.conf!" << std::endl;
      Log << "Erase the second .conf!" << std::endl;
    }
    else if( boost::filesystem::exists( "noggit_template.conf" ) )
    {
      Log << "You must rename noggit_template.conf to noggit.conf if noggit should use the config file!" << std::endl;
    }


#ifdef _WIN32
    Log << "Will try to load the game path from you registry now:" << std::endl;
    HKEY key;
    DWORD t;
    const DWORD s( 1024 );
    char temp[s];
    memset(temp,0,s);
    LONG l = RegOpenKeyEx(HKEY_LOCAL_MACHINE,"SOFTWARE\\Wow6432Node\\Blizzard Entertainment\\World of Warcraft",0,KEY_QUERY_VALUE,&key);
    if (l != ERROR_SUCCESS)
      l = RegOpenKeyEx(HKEY_LOCAL_MACHINE,"SOFTWARE\\Blizzard Entertainment\\World of Warcraft\\PTR",0,KEY_QUERY_VALUE,&key);
    if (l != ERROR_SUCCESS)
      l = RegOpenKeyEx(HKEY_LOCAL_MACHINE,"SOFTWARE\\Blizzard Entertainment\\World of Warcraft",0,KEY_QUERY_VALUE,&key);
    if (l == ERROR_SUCCESS && RegQueryValueEx(key,"InstallPath",0,&t,(LPBYTE)temp,(LPDWORD)&s) == ERROR_SUCCESS)
      return temp;
    else
      return "";
    RegCloseKey(key);
#else
    return "/Applications/World of Warcraft/";
#endif

  }
  else
  {
    Log << "Using config file." << std::endl;
    return ConfigFile( "noggit.conf" ).read<std::string>( "Path" );
  }
}


void Noggit::loadMPQs()
{
  asyncLoader = new AsyncLoader();
  asyncLoader->start(1);

  std::vector<std::string> archiveNames;
  archiveNames.push_back( "" );
  // Models
  archiveNames.push_back("art.MPQ");
  archiveNames.push_back("base-win.MPQ");
  archiveNames.push_back("expansion1.MPQ");
  archiveNames.push_back("expansion2.MPQ");
  archiveNames.push_back("Expansion3.MPQ");
  archiveNames.push_back("sound.MPQ");
  archiveNames.push_back("world.MPQ");
  archiveNames.push_back("world2.MPQ");
  /* Usually the load would be done together to support future patches
  but all patches have been released for Cata
  and WoD will require a whole new loading system*/
  archiveNames.push_back("wow-update-base15211.MPQ");
  archiveNames.push_back("wow-update-base-15354.MPQ");
  archiveNames.push_back("wow-update-base-15595.MPQ");
  //Local
  archiveNames.push_back("{locale}/expansion1-locale-{locale}.MPQ");
  archiveNames.push_back("{locale}/expansion1-speech-{locale}.MPQ");
  archiveNames.push_back("{locale}/expansion2-locale-{locale}.MPQ");
  archiveNames.push_back("{locale}/expansion2-speech-{locale}.MPQ");
  archiveNames.push_back("{locale}/expansion3-locale-{locale}.MPQ");
  archiveNames.push_back("{locale}/expansion3-speech-{locale}.MPQ");
  archiveNames.push_back("{locale}/locale-{locale}.MPQ");
  archiveNames.push_back("{locale}/speech-{locale}.MPQ");
  archiveNames.push_back("{locale}/wow-update-{locale}-15211.MPQ");
  archiveNames.push_back("{locale}/wow-update-{locale}-15354.MPQ");
  archiveNames.push_back("{locale}/wow-update-{locale}-15595.MPQ");

  /* MISTS OF PANDARIA MPQs - Leaving this here incase I ever update to that.
     Will also require a new way of tracking the localization, as the file
	 Realmlist.wtf does not exist in Mists of Pandaria, but is now located in
	 Interface/GlueXML/accountlogin.lua. Most likely just change the file that
	 the system looks for to something else in the folder... Why am I writing
	 this right now? Whose looking at this? Me? Future me? HI ME! Hows the 
	 future? Did WoD turn out any good? How bout Batman v. Superman. I'm
	 really looking forward to that. Did I get a new job? I could really 
	 use a better paying job right now. It's getting really nice weather
	 wise down here. I can't wait for a winter that isn't -50 windchills
	 all goddamn season. WTF am I doing? I'll probably never look at this
	 again. I should really go back to sleep. I'm an idiot. ANDDDDDDDDDDD
	 Committed.
  // Models
  archiveNames.push_back( "alternate.MPQ" );
  archiveNames.push_back( "base-Win.MPQ" );
  archiveNames.push_back( "expansion1.MPQ" );
  archiveNames.push_back( "expansion2.MPQ" );
  archiveNames.push_back( "expansion3.MPQ" );
  archiveNames.push_back( "expansion4.MPQ" );
  archiveNames.push_back( "interface.MPQ" );
  archiveNames.push_back( "itemtexture.MPQ" );
  archiveNames.push_back( "misc.MPQ" );
  archiveNames.push_back( "model.MPQ" );
  archiveNames.push_back( "sound.MPQ" );
  archiveNames.push_back( "texture.MPQ" );
  archiveNames.push_back( "world" );

  Usually the load would be done together to support future patches
  but all patches have been released for Mists
  and WoD will require a whole new loading system

  archiveNames.push_back("wow-update-base-16016.MPQ");
  archiveNames.push_back("wow-update-base-16048.MPQ");
  archiveNames.push_back("wow-update-base-16057.MPQ");
  archiveNames.push_back("wow-update-base-16309.MPQ");
  archiveNames.push_back("wow-update-base-16357.MPQ");
  archiveNames.push_back("wow-update-base-16516.MPQ");
  archiveNames.push_back("wow-update-base-16650.MPQ");
  archiveNames.push_back("wow-update-base-16844.MPQ");
  archiveNames.push_back("wow-update-base-16965.MPQ");
  archiveNames.push_back("wow-update-base-17116.MPQ");
  archiveNames.push_back("wow-update-base-17266.MPQ");
  archiveNames.push_back("wow-update-base-17325.MPQ");
  archiveNames.push_back("wow-update-base-17345.MPQ");
  archiveNames.push_back("wow-update-base-17538.MPQ");
  archiveNames.push_back("wow-update-base-17646.MPQ");
  archiveNames.push_back("wow-update-base-17688.MPQ");
  //Local
  archiveNames.push_back("{locale}/expansion1-speech-{locale}.MPQ");
  archiveNames.push_back("{locale}/expansion2-speech-{locale}.MPQ");
  archiveNames.push_back("{locale}/expansion3-speech-{locale}.MPQ");
  archiveNames.push_back("{locale}/expansion4-speech-{locale}.MPQ");
  archiveNames.push_back("{locale}/locale-{locale}.MPQ");
  archiveNames.push_back("{locale}/speech-{locale}.MPQ");
  archiveNames.push_back("{locale}/wow-update-{locale}-16016.MPQ");
  archiveNames.push_back("{locale}/wow-update-{locale}-16048.MPQ");
  archiveNames.push_back("{locale}/wow-update-{locale}-16057.MPQ");
  archiveNames.push_back("{locale}/wow-update-{locale}-16309.MPQ");
  archiveNames.push_back("{locale}/wow-update-{locale}-16357.MPQ");
  archiveNames.push_back("{locale}/wow-update-{locale}-16516.MPQ");
  archiveNames.push_back("{locale}/wow-update-{locale}-16650.MPQ");
  archiveNames.push_back("{locale}/wow-update-{locale}-16844.MPQ");
  archiveNames.push_back("{locale}/wow-update-{locale}-16965.MPQ");
  archiveNames.push_back("{locale}/wow-update-{locale}-17116.MPQ");
  archiveNames.push_back("{locale}/wow-update-{locale}-17266.MPQ");
  archiveNames.push_back("{locale}/wow-update-{locale}-17325.MPQ");
  archiveNames.push_back("{locale}/wow-update-{locale}-17345.MPQ");
  archiveNames.push_back("{locale}/wow-update-{locale}-17538.MPQ");
  archiveNames.push_back("{locale}/wow-update-{locale}-17646.MPQ");
  archiveNames.push_back("{locale}/wow-update-{locale}-17688.MPQ");
  END MISTS MPQS* /
/*
Leaving WOTLK here for posterity
archiveNames.push_back( "common-2.MPQ" );
  archiveNames.push_back( "expansion.MPQ" );
  archiveNames.push_back( "lichking.MPQ" );
  archiveNames.push_back( "patch.MPQ" );
  archiveNames.push_back( "patch-{number}.MPQ" );
  archiveNames.push_back( "patch-{character}.MPQ" );

  //archiveNames.push_back( "{locale}/backup-{locale}.MPQ" );
  //archiveNames.push_back( "{locale}/base-{locale}.MPQ" );
  archiveNames.push_back( "{locale}/locale-{locale}.MPQ" );
  //archiveNames.push_back( "{locale}/speech-{locale}.MPQ" );
  archiveNames.push_back( "{locale}/expansion-locale-{locale}.MPQ" );
  //archiveNames.push_back( "{locale}/expansion-speech-{locale}.MPQ" );
  archiveNames.push_back( "{locale}/lichking-locale-{locale}.MPQ" );
  //archiveNames.push_back( "{locale}/lichking-speech-{locale}.MPQ" );
  archiveNames.push_back( "{locale}/patch-{locale}.MPQ" );
  archiveNames.push_back( "{locale}/patch-{locale}-{number}.MPQ" );
  archiveNames.push_back( "{locale}/patch-{locale}-{character}.MPQ" );

  archiveNames.push_back( "development.MPQ" );
*/

  const char * locales[] = { "enGB", "enUS", "deDE", "koKR", "frFR", "zhCN", "zhTW", "esES", "esMX", "ruRU" };
  const char * locale( "****" );

  // Find locale, take first one.
  for( int i( 0 ); i < 10; ++i )
  {
    std::string path( wowpath );
    path.append( "Data/" ).append( locales[i] ).append( "/realmlist.wtf" );
    if( boost::filesystem::exists( path ) )
    {
      locale = locales[i];
      Log << "Locale: " << locale << std::endl;
      break;
    }
  }
  if( !strcmp( locale, "****" ) )
  {
    LogError << "Could not find locale directory. Be sure, that there is one containing the file \"realmlist.wtf\"." << std::endl;
    //return -1;
  }

  //! \todo  This may be done faster. Maybe.
  for( size_t i( 0 ); i < archiveNames.size(); ++i )
  {
    std::string path( wowpath );
    path.append( "Data/" ).append( archiveNames[i] );
    std::string::size_type location( std::string::npos );

    do
    {
      location = path.find( "{locale}" );
      if( location != std::string::npos )
      {
        path.replace( location, 8, locale );
      }
    }
    while( location != std::string::npos );

    if( path.find( "{number}" ) != std::string::npos )
    {
      location = path.find( "{number}" );
      path.replace( location, 8, " " );
      for( char j = '2'; j <= '9'; j++ )
      {
        path.replace( location, 1, std::string( &j, 1 ) );
        if( boost::filesystem::exists( path ) )
          MPQArchive::loadMPQ( path, true );
      }
    }
    else if( path.find( "{character}" ) != std::string::npos  )
    {
      location = path.find( "{character}" );
      path.replace( location, 11, " " );
      for( char c = 'a'; c <= 'z'; c++ )
      {
        path.replace( location, 1, std::string( &c, 1 ) );
        if( boost::filesystem::exists( path ) )
          MPQArchive::loadMPQ( path, true );
      }
    }
    else
      if( boost::filesystem::exists( path ) )
        MPQArchive::loadMPQ( path, true );
  }
}

void Noggit::mainLoop()
{
  bool done(false);
  Uint32 ticks(SDL_GetTicks());
  AppState* activeAppState(NULL);
  Uint32 time(0);

  SDL_EnableUNICODE( true );

  SDL_Event event;

  try {

    while( !states.empty() && !done )
    {
      Uint32 lastTicks( ticks );
      ticks = SDL_GetTicks();
      Uint32 tickDelta( ticks - lastTicks );
      time += tickDelta;

      activeAppState = states[states.size() - 1];

      const Uint8 appState( SDL_GetAppState() );
      const bool isActiveApplication( appState & SDL_APPACTIVE );
      const bool hasInputFocus( appState & SDL_APPINPUTFOCUS );
      const bool hasMouseFocus( appState & SDL_APPMOUSEFOCUS );

      if( isActiveApplication )
      {
        const float ftime( time / 1000.0f );
        const float ftickDelta( tickDelta / 1000.0f );
        activeAppState->tick( ftime, ftickDelta );
        activeAppState->display( ftime, ftickDelta );

        video.flip();
      }
      else
      {
        boost::this_thread::sleep( boost::posix_time::milliseconds( 200 ) );
      }

      while( SDL_PollEvent( &event ) )
      {
        if( event.type == SDL_QUIT )
        {
          done = true;
        }
        else if( event.type == SDL_VIDEORESIZE )
        {
          video.resize( event.resize.w, event.resize.h );
          activeAppState->resizewindow();
        }
        else if( hasInputFocus )
        {
          if( ( event.type == SDL_KEYDOWN || event.type == SDL_KEYUP ) )
          {
            activeAppState->keypressed( &event.key );
          }
          else if( hasMouseFocus )
          {
            if( event.type == SDL_MOUSEMOTION )
            {
              activeAppState->mousemove( &event.motion );
            }
            else if( event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP )
            {
              activeAppState->mouseclick( &event.button );
            }
          }
        }
      }

      if( pop )
      {
        pop = false;
        states.pop_back();
        delete activeAppState;
        activeAppState = NULL;
      }
    }

  }
  catch( const std::exception& e )
  {
    LogError << "expection: " << e.what() << std::endl;
  }
}

int Noggit::start(int argc, char *argv[])
{
  InitLogging();
  initPath(argv);

  Log << "Noggit Studio - " << STRPRODUCTVER << std::endl;

  initEnv();
  parseArgs(argc, argv);
  srand(time(NULL));
  wowpath = getGamePath();

  if( wowpath == "" )
    return -1;
  Log << "Game path: " << wowpath << std::endl;

  if(Project::getInstance()->getPath() == "")
    Project::getInstance()->setPath( wowpath );
  Log << "Project path: " << Project::getInstance()->getPath() << std::endl;

  CreateStrips();
  loadMPQs(); // listfiles are not available straight away! They are async! Do not rely on anything at this point!
  OpenDBs();

  if( !video.init( xres, yres, fullscreen, doAntiAliasing ) )
  {
    LogError << "Initializing video failed." << std::endl;
    return -1;
  }

  SDL_WM_SetCaption( "Noggit Studio - " STRPRODUCTVER, "" );
  initFont();

  if(video.mSupportShaders)
    loadWaterShader();
  else
    LogError << "Your GPU does not support ARB vertex programs (shaders). Sorry." << std::endl;

  LogDebug << "Creating Menu" << std::endl;
  states.push_back( new Menu() );

  LogDebug << "Entering Main Loop" << std::endl;
  mainLoop();

  video.close();

  TextureManager::report();
  ModelManager::report();
  WMOManager::report();

  asyncLoader->stop();
  asyncLoader->join();

  MPQArchive::unloadAllMPQs();
  gListfile.clear();

  LogDebug << "Exited" << std::endl;

  return 0;
}

#ifdef _WIN32
int WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
  return main( __argc, __argv );
}
#endif

int main( int argc, char *argv[] )
{
  RegisterErrorHandlers();
  return app.start(argc, argv);
}

//! \todo  Get this out?
//gFileList = new Directory( "root" );
//size_t found;
// This is an example with filter:
/*
 std::vector<std::string>::iterator it;
 for( it = gListfile.begin(); it != gListfile.end(); ++it )
 {
 if( it->find( pFilter ) != std::string::npos )
 {
 found = it->find_last_of("/\\");
 if( found != std::string::npos )
 mDirectory->AddSubDirectory( it->substr(0,found) )->AddFile( it->substr(found+1) );
 else
 mDirectory->AddFile( *it );
 }
 }
 */
// This is an example for getting all files in the list.
/*  std::list<std::string>::iterator it;
 for( it = gListfile.begin(); it != gListfile.end(); ++it )
 {
 found = it->find_last_of("/\\");
 if( found != std::string::npos )
 gFileList->AddSubDirectory( it->substr(0,found) )->AddFile( it->substr(found+1) );
 else
 gFileList->AddFile( *it );
 }
 */
