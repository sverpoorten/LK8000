/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2
   See CREDITS.TXT file for authors and copyrights

   $Id$
*/

#include "externs.h"
#include "InfoBoxLayout.h"
#include "McReady.h"
#include "dlgTools.h"
#include "Atmosphere.h"
#include "RasterTerrain.h"
#include "LKInterface.h"
#include "LKAirspace.h"
#include "RGB.h"
#include "Sideview.h"
#include "LKObjects.h"
#include "Multimap.h"

extern	 int Sideview_iNoHandeldSpaces;
extern	 AirSpaceSideViewSTRUCT Sideview_pHandeled[MAX_NO_SIDE_AS];

double fSplitFact = 0.30;
double fOffset = 0.0;
using std::min;
using std::max;
int k;
double fZOOMScale = 1.0;

extern int XstartScreen, YstartScreen;
extern COLORREF  Sideview_TextColor;
double fHeigtScaleFact;

#define IM_NEAR_AS 2
#define IM_NEXT_WP 1
#define IM_HEADING 0

void MapWindow::RenderAirspace(HDC hdc, const RECT rci) {
  zoom.SetLimitMapScale(false);
  /****************************************************************/
  if (GetSideviewPage() == IM_NEAR_AS)
	return  RenderNearAirspace( hdc,   rci);
  /****************************************************************/


static bool bHeightScale = false;
RECT rc  = rci; /* rectangle for sideview */
bool bInvCol = true; //INVERTCOLORS;
static double fHeigtScaleFact = 1.0;
double fDist = 50.0*1000; // kmbottom
double aclat, aclon, ach, acb, speed, calc_average30s;
double GPSbrg=0;
double wpt_brg;
double wpt_dist;
double wpt_altarriv;
double wpt_altitude;
double wpt_altarriv_mc0;
double calc_terrainalt;
double calc_altitudeagl;
double fMC0 = 0.0f;
int overindex=-1;
bool show_mc0= true;
double fLD;
SIZE tsize;
TCHAR text[80];
TCHAR buffer[80];
BOOL bDrawRightSide =false;
COLORREF GREEN_COL     = RGB_GREEN;
COLORREF RED_COL       = RGB_LIGHTORANGE;
COLORREF BLUE_COL      = RGB_BLUE;
COLORREF LIGHTBLUE_COL = RGB_LIGHTBLUE;
COLORREF col           =  RGB_BLACK;

int *iSplit = &Multimap_SizeY[Get_Current_Multimap_Type()];

#if 0
StartupStore(_T("...Type=%d  CURRENT=%d  Multimap_size=%d = isplit=%d\n"),
	Get_Current_Multimap_Type(), Current_Multimap_SizeY, Multimap_SizeY[Get_Current_Multimap_Type()], *iSplit);
#endif

  if(bInvCol)
	col =  RGB_WHITE;

  HPEN hpHorizon   = (HPEN)  CreatePen(PS_SOLID, IBLSCALE(1), col);
  HBRUSH hbHorizon = (HBRUSH)CreateSolidBrush(col);
  HPEN OldPen      = (HPEN)   SelectObject(hdc, hpHorizon);
  HBRUSH OldBrush  = (HBRUSH) SelectObject(hdc, hbHorizon);


  bool bFound = false;
  SelectObject(hdc, OldPen);
  SelectObject(hdc, OldBrush);
  DeleteObject(hpHorizon);
  DeleteObject(hbHorizon);

  RECT rct = rc; /* rectangle for topview */
  rc.top     = (long)((double)(rci.bottom-rci.top  )*fSplitFact);
  rct.bottom = rc.top ;


  if(bInvCol)
    Sideview_TextColor = INV_GROUND_TEXT_COLOUR;
  else
    Sideview_TextColor = RGB_WHITE;

  SetTextColor(hdc, Sideview_TextColor);

  /****************************************************************/
	  switch(LKevent) {
		case LKEVENT_NEWRUN:
			// CALLED ON ENTRY: when we select this page coming from another mapspace
			fZOOMScale = 1.0;
			fHeigtScaleFact = 1.0;
		break;
		case LKEVENT_UP:
			// click on upper part of screen, excluding center
			if(bHeightScale)
			  fHeigtScaleFact /= ZOOMFACTOR;
			else
			  fZOOMScale /= ZOOMFACTOR;
			break;

		case LKEVENT_DOWN:
			// click on lower part of screen,  excluding center
			if(bHeightScale)
			  fHeigtScaleFact *= ZOOMFACTOR;
			else
		  	  fZOOMScale *= ZOOMFACTOR;
			break;

		case LKEVENT_LONGCLICK:
		     for (k=0 ; k <= Sideview_iNoHandeldSpaces; k++)
			 {
			   if( Sideview_pHandeled[k].psAS != NULL)
			   {
				 if (PtInRect(XstartScreen,YstartScreen,Sideview_pHandeled[k].rc ))
				 {
				   if (EnableSoundModes)PlayResource(TEXT("IDR_WAV_BTONE4"));
				   dlgAirspaceDetails(Sideview_pHandeled[k].psAS);       // dlgA
				   bFound = true;
			//	   LKevent=LKEVENT_NONE;
				 }
			   }
			 }
			 if (PtInRect(XstartScreen, YstartScreen,rc ))
			   bHeightScale = true;
			 if (PtInRect(XstartScreen, YstartScreen,rct ))
			   bHeightScale = false;
	     break;

		case LKEVENT_PAGEUP:
#ifdef OFFSET_SETP
			if(bHeightScale)
			  fOffset -= OFFSET_SETP;
			else
#endif
			{
			  if(*iSplit == SIZE1) *iSplit = SIZE0;
			  if(*iSplit == SIZE2) *iSplit = SIZE1;
			  if(*iSplit == SIZE3) *iSplit = SIZE2;
			}
		break;
		case LKEVENT_PAGEDOWN:
#ifdef OFFSET_SETP
			if(bHeightScale)
			  fOffset += OFFSET_SETP;
			else
#endif
			{
			  if(*iSplit == SIZE2) *iSplit = SIZE3;
			  if(*iSplit == SIZE1) *iSplit = SIZE2;
			  if(*iSplit == SIZE0) *iSplit = SIZE1;
			}
		break;

	  }
	  LKevent=LKEVENT_NONE;

	  // Current_Multimap_SizeY is global, and must be used by all multimaps!
	  // It is defined in Multimap.cpp and as an external in Multimap.h
	  // It is important that it is updated, because we should resize terrain
	  // only if needed! Resizing terrain takes some cpu and some time.
	  // So we need to know when this is not necessary, having the same size of previous
	  // multimap, if we are switching.
	  // The current implementation is terribly wrong by managing resizing of sideview in
	  // each multimap: it should be done by a common layer.
	  // CAREFUL:
	  // If for any reason DrawTerrain() is called after resizing to 0 (full sideview)
	  // LK WILL CRASH with no hope to recover.
	  if(Current_Multimap_SizeY != *iSplit)
	  {
		Current_Multimap_SizeY=*iSplit;
		SetSplitScreenSize(*iSplit);
		rc.top     = (long)((double)(rci.bottom-rci.top  )*fSplitFact);
		rct.bottom = rc.top ;
	  }

	  double hmin = max(0.0, DerivedDrawInfo.NavAltitude-2300);
	  double hmax = max(MAXALTTODAY, DerivedDrawInfo.NavAltitude+1000);
#ifdef OFFSET_SETP
	  if((hmax + fOffset) > 12000.0)
		fOffset -= OFFSET_SETP;
	  if((hmin + fOffset) < 0.0)
		fOffset += OFFSET_SETP;

	  hmin +=  fOffset;
	  hmax +=  fOffset;
#endif

	  if( (hmax *  fHeigtScaleFact) > 15000.0)
		fHeigtScaleFact /= ZOOMFACTOR;

	  if( (hmax *  fHeigtScaleFact) < 200.0)
		fHeigtScaleFact *= ZOOMFACTOR;

	  hmax *=  fHeigtScaleFact;


  if(bInvCol)
  {
    GREEN_COL     = ChangeBrightness(GREEN_COL     , 0.6);
    RED_COL       = ChangeBrightness(RGB_RED       , 0.6);;
    BLUE_COL      = ChangeBrightness(BLUE_COL      , 0.6);;
    LIGHTBLUE_COL = ChangeBrightness(LIGHTBLUE_COL , 0.4);;
  }
  LockFlightData();
  {
    fMC0 = GlidePolar::SafetyMacCready;
    aclat = DrawInfo.Latitude;
    aclon = DrawInfo.Longitude;
    ach   = DrawInfo.Altitude;
    acb    = DrawInfo.TrackBearing;
    GPSbrg = acb;

    acb    = DrawInfo.TrackBearing;
    GPSbrg = DrawInfo.TrackBearing;
    speed = DrawInfo.Speed;

    calc_average30s = DerivedDrawInfo.Average30s;
    calc_terrainalt = DerivedDrawInfo.TerrainAlt;
    calc_altitudeagl = DerivedDrawInfo.AltitudeAGL;
  }
  UnlockFlightData();

  HFONT hfOld = (HFONT)SelectObject(hdc, LK8PanelUnitFont);
  overindex = GetOvertargetIndex();
  wpt_brg = AngleLimit360( acb );
  wpt_dist         = 0.0;
  wpt_altarriv     = 0.0;
  wpt_altarriv_mc0 = 0.0;
  wpt_altitude     = 0.0;
  fMC0 = 0.0;
  fLD  = 0.0;

  if (GetSideviewPage()==IM_NEXT_WP )
  {
    // Show towards target
    if (overindex>=0)
    {
      double wptlon = WayPointList[overindex].Longitude;
      double wptlat = WayPointList[overindex].Latitude;
      DistanceBearing(aclat, aclon, wptlat, wptlon, &wpt_dist, &acb);

      wpt_brg = AngleLimit360(wpt_brg - acb +90.0);
      fDist = max(5.0*1000.0, wpt_dist*1.15);   // 20% more distance to show, minimum 5km
      wpt_altarriv     = WayPointCalc[overindex].AltArriv[ALTA_MC ];
      wpt_altarriv_mc0 = WayPointCalc[overindex].AltArriv[ALTA_MC0];
      wpt_altitude     = WayPointList[overindex].Altitude;
       // calculate the MC=0 arrival altitude



      LockFlightData();
      wpt_altarriv_mc0 =   DerivedDrawInfo.NavAltitude -
        GlidePolar::MacCreadyAltitude( fMC0,
                                       wpt_dist,
                                       acb,
                                       DerivedDrawInfo.WindSpeed,
                                       DerivedDrawInfo.WindBearing,
                                       0, 0, true,
                                       0)  - WayPointList[overindex].Altitude;
      if (IsSafetyAltitudeInUse(overindex)) wpt_altarriv_mc0 -= SAFETYALTITUDEARRIVAL;

      wpt_altarriv =   DerivedDrawInfo.NavAltitude -
        GlidePolar::MacCreadyAltitude( MACCREADY,
                                       wpt_dist,
                                       acb,
                                       DerivedDrawInfo.WindSpeed,
                                       DerivedDrawInfo.WindBearing,
                                       0, 0, true,
                                       0)  - WayPointList[overindex].Altitude;

     
      if ( (DerivedDrawInfo.NavAltitude-wpt_altarriv+wpt_altitude)!=0)
      	fLD = (int) wpt_dist / (DerivedDrawInfo.NavAltitude-wpt_altarriv+wpt_altitude);
      else
	fLD=999;

      if (IsSafetyAltitudeInUse(overindex)) wpt_altarriv -= SAFETYALTITUDEARRIVAL;


      UnlockFlightData();

    }
    else
    {
      // no selected target
      Statistics::DrawNoData(hdc, rc);
      return;
    }
  }
  


  if(fZOOMScale != 1.0)
  {
    if( (fDist *fZOOMScale) > 750000)
	  fZOOMScale /= ZOOMFACTOR;

    if((fDist *fZOOMScale) < 5000)
	  fZOOMScale *= ZOOMFACTOR;
  }
  fDist *=fZOOMScale;
  DiagrammStruct sDia;
  sDia.fXMin =-5000.0f;
  if( sDia.fXMin > (-0.1f * fDist))
	sDia.fXMin = -0.1f * fDist;
  sDia.fXMax = fDist;
  sDia.fYMin = hmin;
  sDia.fYMax = hmax;

  RECT rcc =  rc;
  if(sDia.fYMin < 100)
    rcc.bottom -= BORDER_Y; /* scale witout sea  */
  sDia.rc = rcc;

  RenderAirspaceTerrain( hdc, aclat, aclon,  acb, ( DiagrammStruct*) &sDia );


  int x0 = CalcDistanceCoordinat( 0,  &sDia);
  int y0 = CalcHeightCoordinat  ( 0,  &sDia);

  double xtick = 1.0;
  if (fDist>10.0*1000.0) xtick = 5.0;
  if (fDist>50.0*1000.0) xtick = 10.0;
  if (fDist>100.0*1000.0) xtick = 20.0;
  if (fDist>200.0*1000.0) xtick = 25.0;
  if (fDist>250.0*1000.0) xtick = 50.0;
  if (fDist>500.0*1000.0) xtick = 100.0;

  if(bInvCol)
  {
    SelectObject(hdc, GetStockObject(BLACK_PEN));
    SelectObject(hdc, GetStockObject(BLACK_BRUSH));
  }
  else
  {
    SelectObject(hdc, GetStockObject(WHITE_PEN));
    SelectObject(hdc, GetStockObject(WHITE_BRUSH));
  }

  SetTextColor(hdc, GROUND_TEXT_COLOUR);
  if(bInvCol)
    if(sDia.fYMin > GC_SEA_LEVEL_TOLERANCE)
	  SetTextColor(hdc, INV_GROUND_TEXT_COLOUR);


  if(fSplitFact > 0.0)
  {
  	sDia.rc = rct;
    if (GetSideviewPage() == IM_HEADING)
  	  MapWindow::AirspaceTopView(hdc, &sDia, GPSbrg, 90.0 );

    if (GetSideviewPage() == IM_NEXT_WP)
  	  MapWindow::AirspaceTopView(hdc, &sDia, acb, wpt_brg );
  	sDia.rc = rcc;
  }
  SelectObject(hdc, LK8PanelUnitFont);
  COLORREF txtCol = GROUND_TEXT_COLOUR;
  if(bInvCol)
    if(sDia.fYMin > GC_SEA_LEVEL_TOLERANCE)
    	txtCol = INV_GROUND_TEXT_COLOUR;
  SetBkMode(hdc, TRANSPARENT);
  SetTextColor(hdc, txtCol);

  _stprintf(text, TEXT("%s"),Units::GetUnitName(Units::GetUserDistanceUnit()));
  DrawXGrid(hdc, rci, xtick/DISTANCEMODIFY, xtick, 0,TEXT_ABOVE_LEFT, Sideview_TextColor,  &sDia,text);
  SetTextColor(hdc, Sideview_TextColor);

  double fHeight = sDia.fYMax - sDia.fYMin;
  double  ytick = 10.0;
  if (fHeight >100.0) ytick = 100.0;
  if (fHeight >1000.0) ytick = 500.0;
  if (fHeight >2000.0) ytick = 1000.0;
  if (fHeight >4000.0) ytick = 2000.0;


  if(Units::GetUserAltitudeUnit() == unFeet)
	 ytick = ytick * 4.0;

  _stprintf(text, TEXT("%s"),Units::GetUnitName(Units::GetUserAltitudeUnit()));

  DrawYGrid(hdc, rcc, ytick/ALTITUDEMODIFY,ytick, 0,TEXT_UNDER_RIGHT ,Sideview_TextColor,  &sDia, text);
  POINT line[4];

  // draw target symbolic line


  int iWpPos =  CalcDistanceCoordinat( wpt_dist,  &sDia);
  if (GetSideviewPage() == IM_NEXT_WP)
  {
    if(WayPointCalc[overindex].IsLandable == 0)
    {
      // Not landable - Mark wpt with a vertical marker line
      line[0].x = CalcDistanceCoordinat( wpt_dist,  &sDia);
      line[0].y = y0;
      line[1].x = line[0].x;
      line[1].y = rc.top;
      DrawDashLine(hdc,4, line[0], line[1],  RGB_GREY, rc);
    }
    else
    {
      // Landable
      line[0].x = iWpPos;
      line[0].y = CalcHeightCoordinat( wpt_altitude, &sDia);
      line[1].x = line[0].x;
      line[1].y = CalcHeightCoordinat( SAFETYALTITUDEARRIVAL+wpt_altitude,  &sDia );
      DrawLine(hdc, line[0], line[1],  rc, PS_SOLID,  (5), RGB_ORANGE);
      float fArrHight = 0.0f;
      if(wpt_altarriv > 0.0f)
      {
        fArrHight = wpt_altarriv;
        line[0].x = iWpPos;
        line[0].y = CalcHeightCoordinat( SAFETYALTITUDEARRIVAL+wpt_altitude,  &sDia );
        line[1].x = line[0].x;
        line[1].y = CalcHeightCoordinat( SAFETYALTITUDEARRIVAL+wpt_altitude+fArrHight, &sDia );
        DrawLine(hdc, line[0], line[1],  rc, PS_SOLID,  4, RGB_GREEN);
      }
      // Mark wpt with a vertical marker line
      line[0].x = iWpPos;
      line[0].y = CalcHeightCoordinat( SAFETYALTITUDEARRIVAL+wpt_altitude+fArrHight,  &sDia );
      line[1].x = line[0].x;
      line[1].y = rc.top;
      DrawDashLine(hdc,4, line[0], line[1],  RGB_GREY, rc);
    }
  }


  // Draw estimated gliding line (blue)
 // if (speed>10.0)
  {
    if (GetSideviewPage()== IM_NEXT_WP) {
      double altarriv;

      // Draw estimated gliding line MC=0 (green)
      if( show_mc0 )
      {
        altarriv = wpt_altarriv_mc0 + wpt_altitude;
        if (IsSafetyAltitudeInUse(overindex)) altarriv += SAFETYALTITUDEARRIVAL;
        line[0].x = CalcDistanceCoordinat( 0, &sDia);
        line[0].y = CalcHeightCoordinat ( DerivedDrawInfo.NavAltitude, &sDia );
        line[1].x = CalcDistanceCoordinat( wpt_dist, &sDia);
        line[1].y = CalcHeightCoordinatOutbound( altarriv ,  &sDia );
        DrawDashLine(hdc,3, line[0], line[1],  RGB_BLUE, rc);
      }
      altarriv = wpt_altarriv + wpt_altitude;
      if (IsSafetyAltitudeInUse(overindex)) altarriv += SAFETYALTITUDEARRIVAL;
      line[0].x = CalcDistanceCoordinat( 0, &sDia);
      line[0].y = CalcHeightCoordinat( DerivedDrawInfo.NavAltitude, &sDia );
      line[1].x = CalcDistanceCoordinat( wpt_dist, &sDia);
      line[1].y = CalcHeightCoordinatOutbound( altarriv ,  &sDia );
      DrawDashLine(hdc,3, line[0], line[1],  RGB_BLUE, rc);
    } else {
      double t = fDist/(speed!=0?speed:1);
      if (ISGLIDER || ISPARAGLIDER)

      line[0].x = CalcDistanceCoordinat( 0, &sDia);
      line[0].y = CalcHeightCoordinat  ( DerivedDrawInfo.NavAltitude, &sDia);
      line[1].x = rc.right;
      line[1].y = CalcHeightCoordinat  ( DerivedDrawInfo.NavAltitude+calc_average30s*t, &sDia);
      // Limit climb rate to flat, for free flyers
      if (ISGLIDER || ISPARAGLIDER)
	if ( line[1].y  < line[0].y )  line[1].y  = line[0].y;

      DrawDashLine(hdc,3, line[0], line[1],  RGB_BLUE, rc);
    }
  }


  SelectObject(hdc, GetStockObject(Sideview_TextColor));
  SelectObject(hdc, GetStockObject(WHITE_BRUSH));


  //Draw wpt info texts
  if (GetSideviewPage() == IM_NEXT_WP) {
//HFONT hfOld = (HFONT)SelectObject(hdc, LK8MapFont);
    line[0].x = CalcDistanceCoordinat( wpt_dist,  &sDia);
    // Print wpt name next to marker line

    GetOvertargetName(text);

    GetTextExtentPoint(hdc, text, _tcslen(text), &tsize);
    int x = line[0].x - tsize.cx - NIBLSCALE(5);

    if (x<x0) bDrawRightSide = true;
    if (bDrawRightSide) x = line[0].x + NIBLSCALE(5);
    int y = rc.top + 3*tsize.cy;

    SetTextColor(hdc, Sideview_TextColor);

    /**************************************************
     * draw waypoint target
     **************************************************/
	if (INVERTCOLORS)
	  SelectObject(hdc,LKBrush_Petrol);
	else
	  SelectObject(hdc,LKBrush_LightCyan);
	MapWindow::LKWriteBoxedText(hdc,&MapRect,text,  line[0].x, y-3, 0, WTALIGN_CENTER);

    // Print wpt distance
    Units::FormatUserDistance(wpt_dist, text, 7);
    GetTextExtentPoint(hdc, text, _tcslen(text), &tsize);
    x = line[0].x - tsize.cx - NIBLSCALE(5);
    if (bDrawRightSide) x = line[0].x + NIBLSCALE(5);
    y += tsize.cy;
    ExtTextOut(hdc, x, y, ETO_OPAQUE, NULL, text, _tcslen(text), NULL);
    double altarriv = wpt_altarriv_mc0; // + wpt_altitude;
    if (IsSafetyAltitudeInUse(overindex)) altarriv += SAFETYALTITUDEARRIVAL;



    SetTextColor(hdc, Sideview_TextColor);
    if (wpt_altarriv_mc0 > ALTDIFFLIMIT)
    {
      _stprintf(text, TEXT("Mc %3.1f: "), (LIFTMODIFY*fMC0));
      Units::FormatUserArrival(wpt_altarriv_mc0, buffer, 7);
      _tcscat(text,buffer);
    } else {
      LK_tcsncpy(text, TEXT("---"), sizeof(text)/sizeof(text[0]) - 1);
    }
    GetTextExtentPoint(hdc, text, _tcslen(text), &tsize);
    x = line[0].x - tsize.cx - NIBLSCALE(5);
    if (bDrawRightSide) x = line[0].x + NIBLSCALE(5);   // Show on right side if left not possible
    y += tsize.cy;

    if((wpt_altarriv_mc0 > 0) && (  WayPointList[overindex].Reachable)) {
  	  SetTextColor(hdc, GREEN_COL);
    } else {
  	  SetTextColor(hdc, RED_COL);
    }
    ExtTextOut(hdc, x, y, ETO_OPAQUE, NULL, text, _tcslen(text), NULL);
    
    // Print arrival altitude
    if (wpt_altarriv > ALTDIFFLIMIT) {
      _stprintf(text, TEXT("Mc %3.1f: "), (LIFTMODIFY*MACCREADY));
//      iround(LIFTMODIFY*MACCREADY*10)/10.0
      Units::FormatUserArrival(wpt_altarriv, buffer, 7);
      _tcscat(text,buffer);
    } else {
      LK_tcsncpy(text, TEXT("---"), sizeof(text)/sizeof(text[0]) - 1);
    }

    if(  WayPointList[overindex].Reachable) {
  	  SetTextColor(hdc, GREEN_COL);
    } else {
  	  SetTextColor(hdc, RED_COL);
    }
    GetTextExtentPoint(hdc, text, _tcslen(text), &tsize);
    x = line[0].x - tsize.cx - NIBLSCALE(5);
    if (bDrawRightSide) x = line[0].x + NIBLSCALE(5);   // Show on right side if left not possible
    y += tsize.cy;
    ExtTextOut(hdc, x, y, ETO_OPAQUE, NULL, text, _tcslen(text), NULL);

    // Print arrival AGL
    altarriv = wpt_altarriv;
    if (IsSafetyAltitudeInUse(overindex)) altarriv += SAFETYALTITUDEARRIVAL;
    if(altarriv  > 0)
    {
  	  Units::FormatUserAltitude(altarriv, buffer, 7);
      LK_tcsncpy(text, gettext(TEXT("_@M1742_")), sizeof(text)/sizeof(text[0]) - 1);
      _tcscat(text,buffer);
      GetTextExtentPoint(hdc, text, _tcslen(text), &tsize);
   //   x = CalcDistanceCoordinat(wpt_dist,  rc) - tsize.cx - NIBLSCALE(5);;
      x = line[0].x - tsize.cx - NIBLSCALE(5);
      if (bDrawRightSide) x = line[0].x + NIBLSCALE(5);
      y = CalcHeightCoordinat(  altarriv + wpt_altitude , &sDia );
      if(  WayPointList[overindex].Reachable) {
        SetTextColor(hdc, GREEN_COL);
      } else {
        SetTextColor(hdc, RED_COL);
      }
      ExtTextOut(hdc, x, y-tsize.cy/2, ETO_OPAQUE, NULL, text, _tcslen(text), NULL);
    }
    // Print current Elevation
    SetTextColor(hdc, RGB_BLACK);
    if((calc_terrainalt- hmin) > 0)
    {
  	  Units::FormatUserAltitude(calc_terrainalt, buffer, 7);
      LK_tcsncpy(text, gettext(TEXT("_@M1743_")), sizeof(text)/sizeof(text[0]) - 1);   // ELV:
      _tcscat(text,buffer);
      GetTextExtentPoint(hdc, text, _tcslen(text), &tsize);
      x = CalcDistanceCoordinat(0, &sDia)- tsize.cx/2;
      y = CalcHeightCoordinat(  (calc_terrainalt), &sDia );
      if ((ELV_FACT*tsize.cy) < abs(rc.bottom - y))
      {
        ExtTextOut(hdc, x, rc.bottom -(int)(ELV_FACT * tsize.cy) /* rc.top-tsize.cy*/, ETO_OPAQUE, NULL, text, _tcslen(text), NULL);
      }
    }


    // Print arrival Elevation
    SetTextColor(hdc, RGB_BLACK);
    if((wpt_altitude- hmin) > 0)
    {
  	  Units::FormatUserAltitude(wpt_altitude, buffer, 7);
      LK_tcsncpy(text, gettext(TEXT("_@M1743_")), sizeof(text)/sizeof(text[0]) - 1);   // ELV:
      _tcscat(text,buffer);
      GetTextExtentPoint(hdc, text, _tcslen(text), &tsize);
      x0 = CalcDistanceCoordinat(wpt_dist, &sDia)- tsize.cx/2;
      if(abs(x - x0)> tsize.cx )
      {
        y = CalcHeightCoordinat(  (wpt_altitude), &sDia );
          if ((ELV_FACT*tsize.cy) < abs(rc.bottom - y))
          {
            ExtTextOut(hdc, x0, rc.bottom -(int)(ELV_FACT * tsize.cy) /* rc.top-tsize.cy*/, ETO_OPAQUE, NULL, text, _tcslen(text), NULL);
          }
      }
    }


    if(altarriv  > 0)
    {
    // Print L/D
      _stprintf(text, TEXT("1/%i"), (int)fLD);
      GetTextExtentPoint(hdc, text, _tcslen(text), &tsize);
      SetTextColor(hdc, BLUE_COL);
      x = CalcDistanceCoordinat(wpt_dist/2, &sDia)- tsize.cx/2;
      y = CalcHeightCoordinat( (DerivedDrawInfo.NavAltitude + altarriv)/2 + wpt_altitude , &sDia ) + tsize.cy;
      ExtTextOut(hdc, x, y, ETO_OPAQUE, NULL, text, _tcslen(text), NULL);
    }

    // Print current AGL
    if(calc_altitudeagl - hmin > 0)
    {
      SetTextColor(hdc, LIGHTBLUE_COL);
      Units::FormatUserAltitude(calc_altitudeagl, buffer, 7);
      LK_tcsncpy(text, gettext(TEXT("_@M1742_")), sizeof(text)/sizeof(text[0]) - 1);
      _tcscat(text,buffer);
      GetTextExtentPoint(hdc, text, _tcslen(text), &tsize);
      x = CalcDistanceCoordinat( 0, &sDia) - tsize.cx/2;
      y = CalcHeightCoordinat(  (calc_terrainalt +  calc_altitudeagl)*0.8,   &sDia );
    //    if(x0 > tsize.cx)
          if((tsize.cy) < ( CalcHeightCoordinat(  calc_terrainalt, &sDia )-y)) {
            ExtTextOut(hdc, x, y, ETO_OPAQUE, NULL, text, _tcslen(text), NULL);
          }
    }
    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, hfOld);
//SelectObject(hdc, hfOld);
  } // if Sideview_asp_heading_task


  if (GetSideviewPage()== IM_HEADING)
    wpt_brg =90.0;
  RenderPlaneSideview( hdc,  0.0f, DerivedDrawInfo.NavAltitude,wpt_brg, &sDia );
  HFONT hfOld2 = (HFONT)SelectObject(hdc, LK8InfoNormalFont);
  SetTextColor(hdc, Sideview_TextColor);

  SelectObject(hdc, hfOld2);
  SelectObject(hdc, hfOld);

  SetTextColor(hdc, GROUND_TEXT_COLOUR);

  if(bInvCol)
    if(sDia.fYMin > GC_SEA_LEVEL_TOLERANCE)
	  SetTextColor(hdc, INV_GROUND_TEXT_COLOUR);

  SelectObject(hdc,hfOld/* Sender->GetFont()*/);

  if(fSplitFact > 0.0)
  	sDia.rc = rct;

  hfOld = (HFONT)SelectObject(hdc,LK8InfoNormalFont/* Sender->GetFont()*/);

// DrawTelescope      ( hdc, acb-90.0, rc.right - NIBLSCALE(13),  rc.top   + NIBLSCALE(58));
  DrawNorthArrow     ( hdc,/* GPSbrg*/      acb-90.0     , rct.right - NIBLSCALE(13),  rct.top   + NIBLSCALE(28));
//  RenderBearingDiff( hdc, wpt_brg,  &sDia );

  /****************************************************************************************************
   * draw selection frame
   ****************************************************************************************************/
	if(bHeightScale)
	  DrawSelectionFrame(hdc,  rc);
//	else
//	  DrawSelectionFrame(hdc,  rci);

  SelectObject(hdc,hfOld/* Sender->GetFont()*/);
  zoom.SetLimitMapScale(true);
}





