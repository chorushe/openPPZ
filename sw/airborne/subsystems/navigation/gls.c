/*
 *
 * Copyright (C) 2012, Tobias Münch
 *
 * This file is part of paparazzi.
 *
 * paparazzi is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * paparazzi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with paparazzi; see the file COPYING.  If not, write to
 * the Free Software Foundation, 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * @file subsystems/navigation/gls.c
 * @brief gps landing system   GPS着陆系统
 *
 * gps landing system
 * -automatic calculation of top of decent for const app angle
 *  自动计算计算固定的app角度的合适值
 * -smooth intercept posible
 *  光滑窃听。。。(电台信号滤波)
 * -landing direction is set by app fix / also possible in flight!!!
 *  着陆方向已经经过app设定好了，也可以在飞机中应用
 *
 * in airframe.xml
 * it is possible to define
 * 在airframe.xml文件中它已经被定义好了
 * 1. target_speed   目标速度
 * 2. app_angle  app角
 * 3. app_intercept_af_tod    app 监听电台频率
 *
 * 1 - only efective with useairspeed flag   只有效的用空速标志
 * 2 - defauld is a approach angle of 5 degree which should be fine for most planes   缺省值是接近5度的角，这个角对于大多飞机还是比较好的
 * 3 - distance between approach fix and top of decent  ？？？？？/
 */
 //TOD：transit-oriented development  以公交为导向的开发


#include "generated/airframe.h"
#include "state.h"
#include "subsystems/navigation/gls.h"
#include "subsystems/nav.h"
#include "generated/flight_plan.h"



float target_speed;
float app_angle;
float app_intercept_af_tod;

bool_t init = TRUE;

#ifndef APP_TARGET_SPEED
#define APP_TARGET_SPEED V_CTL_AUTO_AIRSPEED_SETPOINT;
#endif

#ifndef APP_ANGLE
#define APP_ANGLE RadOfDeg(5);
#endif

#ifndef APP_INTERCEPT_AF_TOD
#define APP_INTERCEPT_AF_TOD 100
#endif

//TOD是一个点，这个函数依据af点,td点的坐标计算了TOD点的坐标
static inline bool_t gls_compute_TOD(uint8_t _af, uint8_t _tod, uint8_t _td) {

  if ((WaypointX(_af)==WaypointX(_td))&&(WaypointY(_af)==WaypointY(_td))){
    WaypointX(_af)=WaypointX(_td)-1;
  }

  float td_af_x = WaypointX(_af) - WaypointX(_td);
  float td_af_y = WaypointY(_af) - WaypointY(_td);
  float td_af = sqrt( td_af_x*td_af_x + td_af_y*td_af_y);
  float td_tod = (WaypointAlt(_af) - WaypointAlt(_td)) / (tan(app_angle));

  WaypointX(_tod) = WaypointX(_td) + td_af_x / td_af * td_tod;
  WaypointY(_tod) = WaypointY(_td) + td_af_y / td_af * td_tod;
  WaypointAlt(_tod) = WaypointAlt(_af);

  if (td_tod > td_af) {
    WaypointX(_af) = WaypointX(_tod) + td_af_x / td_af * app_intercept_af_tod;
    WaypointY(_af) = WaypointY(_tod) + td_af_y / td_af * app_intercept_af_tod;
  }
  return FALSE;
}	/* end of gls_copute_TOD */


//###############################################################################################

bool_t gls_init(uint8_t _af, uint8_t _tod, uint8_t _td) {

  init = TRUE;

#if USE_AIRSPEED
  //struct FloatVect2* wind = stateGetHorizontalWindspeed_f();
  //float wind_additional = sqrt(wind->x*wind->x + wind->y*wind->y); // should be gusts only!
  //Bound(wind_additional, 0, 0.5);
  //target_speed = FL_ENVE_V_S * 1.3 + wind_additional; FIXME
  target_speed =  APP_TARGET_SPEED; //  ok for now!
#endif

  app_angle = APP_ANGLE;
  app_intercept_af_tod = APP_INTERCEPT_AF_TOD;
  Bound(app_intercept_af_tod,0,200);


  gls_compute_TOD(_af, _tod, _td);	// calculate Top Of Decent   计算TOD坐标

  return FALSE;
}  /* end of gls_init() */


//###############################################################################################


bool_t gls(uint8_t _af, uint8_t _tod, uint8_t _td) {

  if (init){
#if USE_AIRSPEED
    v_ctl_auto_airspeed_setpoint = target_speed;			// set target speed for approach   设置接近跑道时的速度
#endif
    init = FALSE;
  }

  float final_x = WaypointX(_td) - WaypointX(_tod);
  float final_y = WaypointY(_td) - WaypointY(_tod);
  float final2 = Max(final_x * final_x + final_y * final_y, 1.);

  float nav_final_progress = ((stateGetPositionEnu_f()->x - WaypointX(_tod)) * final_x + (stateGetPositionEnu_f()->y - WaypointY(_tod)) * final_y) / final2;
  Bound(nav_final_progress,-1,1);
  float nav_final_length = sqrt(final2);

  float pre_climb = -(WaypointAlt(_tod) - WaypointAlt(_td)) / (nav_final_length / (*stateGetHorizontalSpeedNorm_f()));
  Bound(pre_climb, -5, 0.);

  float start_alt = WaypointAlt(_tod);
  float diff_alt = WaypointAlt(_td) - start_alt;
  float alt = start_alt + nav_final_progress * diff_alt;
  Bound(alt, WaypointAlt(_td), start_alt +(pre_climb/(-v_ctl_altitude_pgain))) // to prevent climbing before intercept  在侦听之前组织爬行


  if(nav_final_progress < -0.5) {			// for smooth intercept   侦听滤波
    NavVerticalAltitudeMode(WaypointAlt(_tod), 0);	// vertical mode (fly straigt and intercept glideslope)   水平模式（飞直线并且下滑侦听）
    NavVerticalAutoThrottleMode(0);		// throttle mode   油门模式
    NavSegment(_af, _td);				// horizontal mode (stay on localiser)   垂直模式（保持航向）
  }
  else {
    NavVerticalAltitudeMode(alt, pre_climb);	// vertical mode (folow glideslope)     水平模式（跟随下滑）
    NavVerticalAutoThrottleMode(0);		// throttle mode    油门模式
    NavSegment(_af, _td);				// horizontal mode (stay on localiser)   垂直模式（保持航向）
  }

  return TRUE;
}	// end of gls()
