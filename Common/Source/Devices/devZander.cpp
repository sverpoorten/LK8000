/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2
   See CREDITS.TXT file for authors and copyrights

   $Id$
*/

#include "externs.h"

#include "devZander.h"

extern bool UpdateBaroSource(NMEA_INFO* pGPS, const short parserid, const PDeviceDescriptor_t d, const double fAlt);

static BOOL PZAN1(PDeviceDescriptor_t d, TCHAR *String, NMEA_INFO *apGPS);
static BOOL PZAN2(PDeviceDescriptor_t d, TCHAR *String, NMEA_INFO *apGPS);
static BOOL PZAN3(PDeviceDescriptor_t d, TCHAR *String, NMEA_INFO *apGPS);
static BOOL PZAN4(PDeviceDescriptor_t d, TCHAR *String, NMEA_INFO *apGPS);

static BOOL ZanderParseNMEA(PDeviceDescriptor_t d, TCHAR *String, NMEA_INFO *apGPS){
  (void)d;

  if (!NMEAParser::NMEAChecksum(String) || (apGPS == NULL)){
    return FALSE;
  }


  if(_tcsncmp(TEXT("$PZAN1"), String, 6)==0)
    {
      return PZAN1(d, &String[7], apGPS);
    } 
  if(_tcsncmp(TEXT("$PZAN2"), String, 6)==0)
    {
      return PZAN2(d, &String[7], apGPS);
    } 
  if(_tcsncmp(TEXT("$PZAN3"), String, 6)==0)
    {
      return PZAN3(d, &String[7], apGPS);
    } 
  if(_tcsncmp(TEXT("$PZAN4"), String, 6)==0)
    {
      return PZAN4(d, &String[7], apGPS);
    } 
    
  return FALSE;

}


/*
static BOOL ZanderIsLogger(PDeviceDescriptor_t d){
  (void)d;
  return(FALSE);
}
*/


static BOOL ZanderIsGPSSource(PDeviceDescriptor_t d){
  (void)d;
  return(TRUE); 
}


static BOOL ZanderIsBaroSource(PDeviceDescriptor_t d){
	(void)d;
  return(TRUE);
}


static BOOL ZanderLinkTimeout(PDeviceDescriptor_t d){
  (void)d;
  return(TRUE);
}


static BOOL zanderInstall(PDeviceDescriptor_t d){

  _tcscpy(d->Name, TEXT("Zander"));
  d->ParseNMEA = ZanderParseNMEA;
  d->PutMacCready = NULL;
  d->PutBugs = NULL;
  d->PutBallast = NULL;
  d->Open = NULL;
  d->Close = NULL;
  d->Init = NULL;
  d->LinkTimeout = ZanderLinkTimeout;
  d->Declare = NULL;
  d->IsGPSSource = ZanderIsGPSSource;
  d->IsBaroSource = ZanderIsBaroSource;

  return(TRUE);

}


BOOL zanderRegister(void){
  return(devRegister(
    TEXT("Zander"),
    (1l << dfGPS)
    | (1l << dfBaroAlt)
    | (1l << dfSpeed)
    | (1l << dfVario),
    zanderInstall
  ));
}


// *****************************************************************************
// local stuff

static BOOL PZAN1(PDeviceDescriptor_t d, TCHAR *String, NMEA_INFO *apGPS)
{
  TCHAR ctemp[80];
  NMEAParser::ExtractParameter(String,ctemp,0);
  UpdateBaroSource( apGPS, 0,d, AltitudeToQNHAltitude( StrToDouble(ctemp, NULL)));
  return TRUE;
}


static BOOL PZAN2(PDeviceDescriptor_t d, TCHAR *String, NMEA_INFO *apGPS)
{
  TCHAR ctemp[80];
  double vtas, wnet, vias;

  NMEAParser::ExtractParameter(String,ctemp,0);
  vtas = StrToDouble(ctemp,NULL)/3.6;
  // JMW 20080721 fixed km/h->m/s conversion
  
  NMEAParser::ExtractParameter(String,ctemp,1);
  wnet = (StrToDouble(ctemp,NULL)-10000)/100; // cm/s
  apGPS->Vario = wnet;


  if (apGPS->BaroAltitudeAvailable)
  {
    vias = vtas/AirDensityRatio(apGPS->BaroAltitude);
  } else {
    vias = 0.0;
  }

  apGPS->AirspeedAvailable = TRUE;
  apGPS->TrueAirspeed = vtas;
  apGPS->IndicatedAirspeed = vias;
  apGPS->VarioAvailable = TRUE;

  TriggerVarioUpdate();

  return TRUE;
}

static BOOL PZAN3(PDeviceDescriptor_t d, TCHAR *String, NMEA_INFO *apGPS)
{
  //$PZAN3,+,026,A,321,035,V*cc
  //Windkomponente (+=Rückenwind, -=Gegenwind)
  //A=active (Messung Windkomponente ok) / V=void (Messung nicht verwendbar)
  //Windrichtung (true, Wind aus dieser Richtung))
  //Windstärke (km/h)
  //A=active (Windmessung ok) / V=void (Windmessung nicht verwendbar)
  //Windmessung im Geradeausflug: mit ZS1-Kompass A,A, ohne Kompass A,V
  //Windmessung im Kreisflug: V,A
    
  TCHAR ctemp[80];
  double wspeed, wfrom;
  char wind_usable;
   
  NMEAParser::ExtractParameter(String,ctemp,3);
  wfrom=StrToDouble(ctemp,NULL);
  
  NMEAParser::ExtractParameter(String,ctemp,4);
  wspeed=StrToDouble(ctemp,NULL);
  
  NMEAParser::ExtractParameter(String,ctemp,5);
  wind_usable=ctemp[0]; 
  

  if (wind_usable == 'A') {

	wspeed/=3.6;

	#if 1 // 120424 fix correct wind setting

	apGPS->ExternalWindAvailable = TRUE;
	apGPS->ExternalWindSpeed = wspeed;
	apGPS->ExternalWindDirection = wfrom;

	#else

	// do not update if it has not changed
	if ( (wspeed!=CALCULATED_INFO.WindSpeed) || (wfrom != CALCULATED_INFO.WindBearing) ) {

		SetWindEstimate(wspeed, wfrom,9);
		CALCULATED_INFO.WindSpeed=wspeed;
		CALCULATED_INFO.WindBearing=wfrom;

	}
	#endif
  }
    
  return true;
}

static BOOL PZAN4(PDeviceDescriptor_t d, TCHAR *String, NMEA_INFO *apGPS)
{
  //$PZAN4,1.5,+,20,39,45*cc
  //Einstellungen am ZS1:
  //MacCready (m/s)
  //windcomponent (km/h)
  //wing loading (kp/m2)
  //best glide ratio
    
  TCHAR ctemp[80];
  
  NMEAParser::ExtractParameter(String,ctemp,0);
  MACCREADY = StrToDouble(ctemp,NULL);
   
  
  return true;
}


