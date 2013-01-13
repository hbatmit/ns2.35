
/*
 * ew.cc
 * Copyright (C) 1999 by the University of Southern California
 * $Id: ew.cc,v 1.8 2010/03/08 05:54:49 tom_henderson Exp $
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * The copyright of this module includes the following
 * linking-with-specific-other-licenses addition:
 *
 * In addition, as a special exception, the copyright holders of
 * this module give you permission to combine (via static or
 * dynamic linking) this module with free software programs or
 * libraries that are released under the GNU LGPL and with code
 * included in the standard release of ns-2 under the Apache 2.0
 * license or under otherwise-compatible licenses with advertising
 * requirements (or modified versions of such code, with unchanged
 * license).  You may copy and distribute such a system following the
 * terms of the GNU GPL for this module and the licenses of the
 * other code concerned, provided that you include the source code of
 * that other code when and as the GNU GPL requires distribution of
 * source code.
 *
 * Note that people who make modified versions of this module
 * are not obligated to grant this special exception for their
 * modified versions; it is their choice whether to do so.  The GNU
 * General Public License gives permission to release a modified
 * version without this exception; this exception also makes it
 * possible to release a modified version which carries forward this
 * exception.
 *
 */

//
// ew.cc (Early warning system)
//   by Xuan Chen (xuanc@isi.edu), USC/ISI

#include "ip.h"
#include "tcp.h"
#include "tcp-full.h"
#include "random.h"

#include "ew.h"

// Definition of High-Low Filter
HLF::HLF() {
  alpha = 0;
  high = low = 0;
}

void HLF::reset(double value) {
  high = low = value;
}

void HLF::reset() {
  reset(0);
}

// Set Alpha
void HLF::setAlpha(double value) {
  if (value > 1 || value < 0)
    return;

  if (value >= 0.5)
    alpha = value;
  else
    alpha = 1 - value;
}

// Get outputs from HLF
double HLF::getHigh() {
  return(high);
}

double HLF::getLow() {
  return(low);
}

// update high-low filter
// high(t) = alpha * high(t-1) + (1 - alpha) * o(t)
// low(t) = (1 - alpha) * low(t-1) + alpha * o(t)
void HLF::update(double input) {
  // Set values to the current observation for the first time.
  if (high)
  	high = alpha * high + (1 - alpha) * input;
  else
  	high = input;
  if (low)
	low = (1 - alpha) * low + alpha * input;
  else
  	low = input;
  //  printf("HLF %d %.2f, %.2f\n", (int)now, high, low);
}

// Definition for a token-bucket rate limitor
TBrateLimitor::TBrateLimitor() {
  TBrateLimitor(DEFAULT_TB_RATE_P);
}

TBrateLimitor::TBrateLimitor(double rate) {
  double now = Scheduler::instance().clock();
  pkt_mode = 1;
  bucket_size = DEFAULT_TB_SIZE;
  token_rate = 0;
  token_num = bucket_size;
  last_time = now;
  ini_token_rate = rate;

  resetScore();
  setRate(rate);
  //printf("TB pkt_mode:%d, bucket_size:%g, token_num:%g, last_time:%g\n",
  //	 pkt_mode, bucket_size, token_num, last_time);

  // High-low filter
  hlf.setAlpha(ALPHA);
  hlf.reset(rate);
}

// adjust the rate limit to the default value
void TBrateLimitor::setRate(double rate) {
  last_token_rate = token_rate;
  
  if (! token_rate) {
    token_rate = rate;
  } else if (rate != token_rate) {
    // use HLF to change token rate
    hlf.update(rate);
    // Use low-gain filter for fast response
    //token_rate = hlf.getLow();
    token_rate = hlf.getHigh();
    token_rate = rate;
  }
  
  printf("TR %d %.2f %.2f %d %d\n", (int)(Scheduler::instance().clock()), 
	 token_rate, rate, p_score, n_score);
}

// adjust the rate limit to approaching an optimal rate limit
void TBrateLimitor::adjustRate() {
  // pay the penalty
  adjustScore(-1);

  double rate = token_rate;
  if (p_score >= n_score)
    rate = token_rate * (1 + 0.2);
  else 
    rate = token_rate * (1 - 0.2);

  setRate(rate);
}

// Reset negative / positive score
void TBrateLimitor::resetScore() {
  n_score = p_score = 0;
}

// adjust the score for increasing or decreasing scores
void TBrateLimitor::adjustScore(int score) {
  // pay the penalty
  if (last_token_rate > token_rate)
    n_score += score;
  else
    p_score += score;
}

int TBrateLimitor::run(double incoming, double t_rate) {
  double now = Scheduler::instance().clock();
  double interval = now - last_time;
  
  //printf("TB: now:%g last_time:%g interval:%g; ", now, last_time, interval);
  token_num += interval * t_rate;
  last_time = now;

  // more tokens are overflowed
  if (token_num > bucket_size)
    token_num = bucket_size;

  //printf("token #:%g; ", token_num);

  // through (0 dropping probability)
  if (token_num >= incoming) {
    token_num -= incoming;
    //printf("...through\n");
    return 0;
  }

  // dropped
  //printf("...dropped\n");
  return 1;
}

int TBrateLimitor::run(double incoming) {
  return (run(incoming, token_rate));
}

// EW detector
// Constructor
EWdetector::EWdetector() {
  ew_src = ew_dst = -1;

  // reset measurement
  cur_rate = avg_rate = 0;

  // reset timers
  db_timer = dt_timer = 0;

  // reset alarm
  resetAlarm();
  resetChange();

  // High-low filter
  hlf.setAlpha(ALPHA);
}

//EWdetector::~EWdetector() {
//};

// Enable detecting and debugging
void EWdetector::setDt(int inv) {
  dt_inv = inv;
  //printf("DT: %d\n", dt_inv);
}

void EWdetector::setDb(int inv) {
  db_inv = inv;
  //printf("DB: %d\n", db_inv);
}

void EWdetector::setLink(int src, int dst) {
  ew_src = src;
  ew_dst = dst;
  //printf("EW: (%d:%d)\n", ew_src, ew_dst);
}

void EWdetector::setAlarm() {
  alarm = 1;

  // Reset low and high gain filters' input values to the long-term avg
  // Actually, there is no change to high gain filter
  hlf.reset(avg_rate);
}

void EWdetector::resetAlarm() {
  alarm = 0;

  // Reset low and high gain filters' input values to the long-term avg
  // Actually, there is no change to low gain filter
  hlf.reset(avg_rate);
}

// Set and reset change flag
void EWdetector::setChange() {
  change = 1;
}

void EWdetector::resetChange() {
  change = 0;
}

// Test if the alarm has been triggered
int EWdetector::testAlarm() {
  if (!change)
    return(EW_UNCHANGE);
  else 
    return(alarm);
}

// Update long term average
void EWdetector::updateAvg() {
  // update the request rate
  // update the aggregated response rate
  // Update flip-flop filter
  hlf.update(cur_rate);
  
  // Update SWIN, not used any more.
  //updateSWin(cur_rate);
  //ravgSWin();
  //printSWin();
  
  // Update the long term average value with the output from different filters
  if (!alarm) {
    // Use low-gain filter for fast response
    //avg_rate = hlf.getLow();
    avg_rate = hlf.getHigh();
  } else {
    // Use high-gain filter to keep the long term average stable
    avg_rate = hlf.getHigh();
  }
}

// the detector's engine
void EWdetector::run(Packet *pkt) {
  // get the time
  now = Scheduler::instance().clock();
  
  //printf("EW[%d:%d] run ", ew_src, ew_dst);
  // update the measurement 
  measure(pkt);

  // There is a timeout!
  if (now >= dt_timer) {
    // Start detection
    //printf("Detection timeout(%d)\n", (int)now);
    
    // 1. Update the current rate from measurement
    updateCur();

    // 2. Detect change and Trigger alarm if necessary
    // Compare the current rate with the long term average
    detect();
    
    // 3. Update the long term averages
    updateAvg();

    // setup the sleeping timer
    dt_timer = (int)now + dt_inv;
    //printf("%d\n", dt_inv);

    change = 1;
  }
  
  // Schedule debug
  if (db_inv && now >= db_timer) {
    //printf("debugB ");
    trace();
    db_timer = (int)now + db_inv;
  }
}

// end of EW detector

// EW bit rate detector
//Constructor.  
EWdetectorB::EWdetectorB() : EWdetector() {
  drop_p = 0;
  arr_count = 0;

  adjustor = 1.0;

  // Initialize ALIST
  alist.head = alist.tail = NULL;
  alist.count = 0;

  swin.head = swin.tail = NULL;
  swin.count = swin.ravg = 0;
}

//Deconstructor.
EWdetectorB::~EWdetectorB(){
  resetAList();
  resetSWin();
}

// Initialize the EW parameters
void EWdetectorB::init(int ew_adj) {
  // EW adjustor (g = resp rate / request rate)
  adjustor = ew_adj;
}

// Update current measurement 
void EWdetectorB::measure(Packet *pkt) {
  //printf(" before UA");
  // Conduct detection continously
  updateAList(pkt);
  //printf(" after UA");
}

// Update current measurement 
void EWdetectorB::updateCur() {
  //printAList();
  // Record current aggregated response rate
  cur_rate = computeARR();
}

// Check if the packet belongs to existing flow
int EWdetectorB::exFlow(Packet *) {
  // Should check SYN packets to protect existing connections
  //   need to use FullTCP
  return(0);
}

// Conduct the measurement
void EWdetectorB::updateAList(Packet *pkt) {
  hdr_cmn* hdr = hdr_cmn::access(pkt);
  hdr_ip* iph = hdr_ip::access(pkt);
  int dst_id = iph->daddr();
  int src_id = iph->saddr();
  int f_id = iph->flowid(); 

  // Get the corresponding id.
  //printf("EW[%d:%d] in detector\n", ew_src, ew_dst);

  AListEntry *p;
  p = searchAList(src_id, dst_id, f_id);

  // Add new entry to AList
  // keep the bytes sent by each aggregate in AList
  if (!p) {
    p = newAListEntry(src_id, dst_id, f_id);
  }

  // update the existing (or just created) entry in AList
  assert(p && p->f_id == f_id && p->src_id == src_id && p->dst_id == dst_id);

  // update the flow's arrival rate using TSW
  double bytesInTSW, newBytes;
  bytesInTSW = p->avg_rate * p->win_length;
  newBytes = bytesInTSW + (double) hdr->size();
  p->avg_rate = newBytes / (now - p->t_front + p->win_length);
  p->t_front = now;

  //printAListEntry(p, 0);
}

// Get the median for a part of AList 
//   starting from index with count entries
int EWdetectorB::getMedianAList(int index, int count) {
  int m;
  
  if (!count)
    return 0;

  sortAList();
  //printAList();

  // Pick the entry with median avg_rate
  m = (int) (count / 2);
  if (2 * m == count) {
    return((getRateAList(index + m - 1) + getRateAList(index + m)) / 2);
  } else {
    return(getRateAList(index + m));
  }
}

// Get the rate given the index in the list
int EWdetectorB::getRateAList(int index) {
  struct AListEntry *p;

  //printf("%d\n", index);
  p = alist.head;
  for (int i = 0; i < index; i++) {
    if (p)
      p = p->next;
  }
  
  if (p)
    return ((int)p->avg_rate);

  printf("Error in AList!\n");
  return(0);
}

// Calculate the aggragated response rate for high-bandwidth flows
int EWdetectorB::computeARR() {
  int i, agg_rate;

  // Explicit garbage collection first 
  //  before both choosing HBFs and searching AList
  //printf("before timeout ");
  timeoutAList();
  //printf("after timeout ");

  // do nothing if no entry exists
  if (!alist.count) 
    return 0;

  // Pick the 10% highest bandwidth flows
  arr_count = (int) (alist.count * 0.1 + 1);

  // Sort AList first
  sortAList();

  // Calculate the ARR for HBFs
  // Use mean
  agg_rate = 0;
  for (i = 0; i < arr_count; i++) {
    agg_rate += getRateAList(i);
  }
  
  if (i)
    agg_rate = (int) (agg_rate / i);
  else {
    printf("No MAX returned from ALIST!!!\n");
  }
  
  // Use median (the median for the list or median for HBFs?)
  //agg_rate = getMedianAList(0, k);
  //printf("%f %d %d %d\n", now, k, agg_rate, getMedianAList(0, k));
  
  return(agg_rate);
}

// Find the matched AList entry
struct AListEntry * EWdetectorB::searchAList(int src_id, int dst_id, int f_id){
  AListEntry *p;

  // Explicit garbage collection first.
  //printf("before timeout ");
  timeoutAList();
  //printf("after timeout ");
  // Use src and dest pair and flow id:
  //   aggregate flows within the same request-response exchange
  // Timeout need to be set to a very small value in order to
  //   seperate different exchanges.
  p = alist.head;
  while (p && 
	 (p->f_id != f_id || p->src_id != src_id || p->dst_id != dst_id)) {
    p = p->next;
  }
  
  return(p);
}

// Add new entry to AList
struct AListEntry * EWdetectorB::newAListEntry(int src_id, int dst_id, int f_id) {
  AListEntry *p;

  p = new AListEntry;
  p->src_id = src_id;
  p->dst_id = dst_id;
  p->f_id = f_id;
  p->last_update = now;
  p->avg_rate = 0;
  // Since we are doing random sampling, 
  // the t_front should set to the beginning of this period instead of 0.
  p->t_front = now;
  p->win_length = 1;
  p->next = NULL;
  
  // Add new entry to AList
  if (alist.tail)
    alist.tail->next = p;
  alist.tail = p;
  
  if (!alist.head)
    alist.head = p;
  
  alist.count++;

  return(p);
}

// Find the entry with max avg_rate in AList
struct AListEntry * EWdetectorB::getMaxAList() {
  struct AListEntry *p, *pp, *max, *pm;

  //printAList();
  // find the max entry and remove
  p = pp = alist.head;
  max = pm = p;
  
  while (p) {
    if (p->avg_rate > max->avg_rate) {
      pm = pp;
      max = p;
    }
    
    pp = p;
    p = p->next;
  }
  
  // remove max from AList
  if (alist.head == max)
    alist.head = max->next;
  
  if (pm != max)
    pm->next = max->next;
  
  max->next = NULL;
  //printAList();

  return(max);
}

// Sort AList based on the avg_rate
void EWdetectorB::sortAList() {
  struct AListEntry *max, *head, *tail;
  
  if (!alist.head)
    return;

  //printAList();
  head = tail = NULL;

  while (alist.head) {
    // Get the entry with the max avg_rate
    max = getMaxAList();
    //printAListEntry(max, i);
    
    if (max) {
      // Add max to the tail of the new list
      if (tail)
	tail->next = max;
      tail = max;
      
      if (!head)
	head = max;
    }
  }

  alist.head = head;
  alist.tail = tail;

  //printAList();
}

// Timeout AList entries
void EWdetectorB::timeoutAList() {
  AListEntry *p, *q;
  float to;

  to = EW_FLOW_TIME_OUT;
  if (dt_inv)
    to = dt_inv;

  // Expire the old entries in AList
  p = q = alist.head;
  while (p) {
    // Garbage collection
    if (p->last_update + to < now){
      // The coresponding flow is expired.      
      if (p == alist.head){
	if (p == alist.tail) {
	  alist.head = alist.tail = NULL;
	  free(p);
	  p = q = NULL;
	} else {
	  alist.head = p->next;
	  free(p);
	  p = q = alist.head;
	}
      } else {
	q->next = p->next;
	if (p == alist.tail)
	  alist.tail = q;
	free(p);
	p = q->next;
      }
      alist.count--;
    } else {
      q = p;
      p = q->next;
    }
  }
}

// Reset AList
void EWdetectorB::resetAList() {
  struct AListEntry *ap, *aq;

  ap = aq = alist.head;
  while (ap) {
    aq = ap;
    ap = ap->next;
    free(aq);
  }
  
  ap = aq = NULL;
  alist.head = alist.tail = NULL;  
  alist.count = 0;
}



// Reset SWin
void EWdetectorB::resetSWin() {
  struct SWinEntry *p, *q;

  p = q = swin.head;
  while (p) {
    q = p;
    p = p->next;
    free(q);
  }
  
  p = q = NULL;
  swin.head = swin.tail = NULL;  
  swin.count = swin.ravg = 0;
}

// update swin with the latest measurement of aggregated response rate
void EWdetectorB::updateSWin(int rate) {
  struct SWinEntry *p, *new_entry;

  new_entry = new SWinEntry;
  new_entry->rate = rate;
  new_entry->weight = 1;
  new_entry->next = NULL;
  
  if (swin.tail)
    swin.tail->next = new_entry;
  swin.tail = new_entry;
  
  if (!swin.head)
    swin.head = new_entry;

  // Reset current rate.
  if (swin.count < EW_SWIN_SIZE) {
    swin.count++;
  } else {
    p = swin.head;
    swin.head = p->next;
    free(p);
  }
}

// Calculate the running average over the sliding window
void EWdetectorB::ravgSWin() {
  struct SWinEntry *p;
  float sum = 0;
  float t_weight = 0;

  //printf("Calculate running average over the sliding window:\n");
  p = swin.head;
  //printf("after p\n");

  while (p) {
    //printSWinEntry(p, i++);
    sum += p->rate * p->weight;
    t_weight += p->weight;
    p = p->next;
  }
  p = NULL;
  //printf("\n");  

  swin.ravg = (int)(sum / t_weight);

  //  printf("Ravg: %d\n", swin.ravg);
}

// detect the traffic change by 
// comparing the current measurement with the long-term average
//   trigger alarm if necessary.
void EWdetectorB::detect() {
  // When ALARM:
  //  detect if it is the time to release the alarm
  // When NO ALARM:
  //  detect if it is the time to trigger the alarm
  if (alarm) {
    // Determine if an alarm should be released
    if (cur_rate > avg_rate * (1 + EW_RELEASE_RANGE)) {
      // reset alarm
      resetAlarm();
    } 
  } else {
    // Determine if an alarm should be triggered
    //   need to be conservative!
    if (cur_rate < avg_rate * (1 - EW_DETECT_RANGE)) {
      setAlarm();
      
      // Initial drop_p to the MAX value whenever alarm triggered
      if (drop_p < EW_MAX_DROP_P)
	drop_p = EW_MAX_DROP_P;
    } else {
    }
  }
  
  // Determine the dropping probability
  //computeDropP();
}

// Determine the dropping probability based on current measurement
void EWdetectorB::computeDropP() {
  double p = 0;

  if (alarm) {
    // Compute the dropping probability as a linear function of current rate
    //  p is computed towards the current measurement.
    p = 1;
    if (cur_rate)
      p = (avg_rate - cur_rate) * adjustor / cur_rate;
    
    // p could be greater than 1
    if (p > 1)
      p = 1;
    // p could also be negative
    if (p < 0)
      p = 0;
    
    // Compute the actual drop probability
    drop_p = ALPHA * drop_p + (1 - ALPHA) * p;    
    // adjust drop_p
    if (drop_p < EW_MIN_DROP_P)
      drop_p = EW_MIN_DROP_P;
    if (drop_p > EW_MAX_DROP_P)
      drop_p = EW_MAX_DROP_P;
  } else {
    // Fade out the drop_p when no alarm
    if (drop_p > 0) {
      if (drop_p <= EW_MIN_DROP_P)
        drop_p = 0;
      else {
        drop_p = ALPHA * drop_p;
      }
    }
  }
}

// Decreas the sample interval
void EWdetectorB::decSInv() {
  // Need some investigation for the min allowed detection interval
  //  if (s_inv / 2 > EW_MIN_SAMPLE_INTERVAL) {
  // s_inv = s_inv / 2;
    
    //printf("SINV decreased by 2.\n");
  //}
}

// Increase the sample interval
void EWdetectorB::incSInv() {
  //if(s_inv * 2 <= init_s_inv) {
  //  s_inv = s_inv * 2;
  
  //printf("SINV increased by 2.\n");
  // }
}

// Prints one entry in SWin
void EWdetectorB::printSWin() {
  struct SWinEntry *p;
  printf("%f SWIN[%d, %d]", now, swin.ravg, swin.count);
  p = swin.head;
  int i = 0;
  while (p) {
    printSWinEntry(p, i++);
    p = p->next;
  }
  p = NULL;
  printf("\n");
}

// Print the contents in SWin
void EWdetectorB::printSWinEntry(struct SWinEntry *p, int i) {
  if (p)
    printf("[%d: %d %.2f] ", i, p->rate, p->weight);
}

// Print one entry in AList
void EWdetectorB::printAListEntry(struct AListEntry *p, int i) {
  if (!p)
    return;

  printf("[%d] %d (%d %d) %.2f %.2f\n", i, p->f_id, p->src_id, p->dst_id, 
	 p->avg_rate, p->last_update);
}


// Print the entries in AList
void EWdetectorB::printAList() {
  struct AListEntry *p;
  printf("%f AList(%d):\n", now, alist.count);

  p = alist.head;
  int i = 0;
  while (p) {
    printAListEntry(p, i);
    i++;
    p = p->next;
  }
  p = NULL;
  printf("\n");
}

// Trace bit rate (resp rate)
void EWdetectorB::trace() {
  double db_rate = 0;
  double m_rate = 0;

  timeoutAList();
  m_rate = getMedianAList(0, alist.count);
  //printf("B ");
  db_rate = computeARR();

  if (!m_rate || !db_rate)
  {
	  ;//printAList();
  }

  printf("B %d %.2f %.2f %d %d %.2f %.2f\n", 
	 (int)now, cur_rate, avg_rate, arr_count, alarm, db_rate, m_rate);
}

// EW packet detector
EWdetectorP::EWdetectorP() : EWdetector() {
  // Packet stats
  cur_p.arrival = cur_p.dept = cur_p.drop = 0;
  last_p.arrival = last_p.dept = last_p.drop = 0;
  last_p_db.arrival = last_p_db.dept = last_p_db.drop = 0;
}

EWdetectorP::~EWdetectorP(){
  // Packet stats
  cur_p.arrival = cur_p.dept = cur_p.drop = 0;
  last_p.arrival = last_p.dept = last_p.drop = 0;
}

// get the current request rate
double EWdetectorP::getRate() {
  return(cur_rate);
}

// update packet stats
void EWdetectorP::updateStats(int flag) {
  // Packet arrival
  if (flag == PKT_ARRIVAL) {
    cur_p.arrival++;
    return;
  }

  // Packet departure
  if (flag == PKT_DEPT) {
    cur_p.dept++;
    return;
  }

  // Packet dropped
  if (flag == PKT_DROP) {
    cur_p.drop++;
    return;
  }
}

// Detect changes in packet rate
void EWdetectorP::detect() {
  if (cur_rate > avg_rate * (1 + EW_DETECT_RANGE)) {
    if (!alarm) {
      setAlarm();
    }
  } else if (cur_rate < avg_rate * (1 - EW_RELEASE_RANGE)) {
    if (alarm)
      resetAlarm();
  }
}

// Update current measurement
void EWdetectorP::updateCur() {
  // measure the accepted packet rate (rather than arrival rate)
  cur_rate = (cur_p.dept - last_p.dept) / dt_inv;

  // keep the current value
  last_p.arrival = cur_p.arrival;
  last_p.dept = cur_p.dept;
  last_p.drop = cur_p.drop;
}

// Update long term average
/*void EWdetectorP::updateAvg() {
  avg_rate = (int)(hlf.alpha * avg_rate + (1 - hlf.alpha) * cur_rate);
}
*/
// Update stats
void EWdetectorP::measure(Packet *) {
  

  // stats on packet departure and drop are collect in policer 
}

// Trace packet incoming rate (req rate)
void EWdetectorP::trace() {
  printf("P %d %.2f %.2f %d %d %d %d %d %d %d\n", 
	 (int)now, cur_rate, avg_rate, alarm,
	 cur_p.arrival - last_p_db.arrival,
	 cur_p.dept - last_p_db.dept,
	 cur_p.drop - last_p_db.drop,  
	 cur_p.arrival, cur_p.dept, cur_p.drop);

  last_p_db.arrival = cur_p.arrival;
  last_p_db.dept = cur_p.dept;
  last_p_db.drop = cur_p.drop;
}

// EW Policy: deal with queueing stuffs.
//Constructor.  
EWPolicy::EWPolicy() : Policy() {
  // Initialize detectors
  ewB = cewB = NULL;
  ewP = cewP = NULL;
  
  // Initialize rate limitor
  rlP = rlB = NULL;

  max_p = max_b = 0;
  alarm = pre_alarm = 0;
  change = 0;
}

//Deconstructor.
EWPolicy::~EWPolicy(){
  if (ewB)
    free(ewB);

  if (ewP)
    free(ewP);

  if (cewB)
    free(cewB);

  if (cewP)
    free(cewP);
}

// Initialize the EW parameters
void EWPolicy::init(int adj, int src, int dst) {
  ew_adj = adj;
  qsrc = src;
  qdst = dst;
}

// EW meter: do nothing.
//  measurement is done in policer: we need to know whether the packet is
//    dropped or not.
void EWPolicy::applyMeter(policyTableEntry *, Packet *) {
  return;
}

// EW Policer
//  1. do measurement: P: both arrival and departure; B: only departure
//  2. make packet drop decisions
int EWPolicy::applyPolicer(policyTableEntry *, policerTableEntry *policer, Packet *pkt) {
  //printf("enter applyPolicer ");

  // can't count/penalize ACKs:
  //   with resp: may cause inaccurate calculation with TSW(??)
  //   with req:  may cause resp retransmission.
  // just pass them through
  hdr_cmn *th = hdr_cmn::access(pkt);
  hdr_ip* iph = hdr_ip::access(pkt);
  int dst_id = iph->daddr();
  int src_id = iph->saddr();

  if (th->ptype() == PT_ACK)
    return(policer->initialCodePt);

  // for other packets...

  // Get time
  now = Scheduler::instance().clock();

  // keep arrival packet stats
  if (ewP) {
    printf("TRR %d %d %d %d\n", (int)now, src_id, dst_id, th->size());
    ewP->updateStats(PKT_ARRIVAL);
  }

  // For other packets:
  if (dropPacket(pkt)) {
    // keep packet stats
    if (ewP)
      ewP->updateStats(PKT_DROP);
    
    //printf("downgrade!\n");	
    return(policer->downgrade1);
  } else {
    // keep packet stats
    if (ewP)
      ewP->updateStats(PKT_DEPT);

    // conduct EW detection
    if (ewP)
      ewP->run(pkt);
    
    if (ewB)
      ewB->run(pkt);    

    //printf("initial!\n");	
    return(policer->initialCodePt);
  }
}

// detect if there is alarm triggered
void EWPolicy::detect(Packet *) {
  int alarm_b, alarm_p;

  alarm_b = alarm_p = 0;

  if (!ewP || ! cewB)
    return;
  
  alarm_b = cewB->testAlarm();
  alarm_p = ewP->testAlarm();
  

  if (alarm_p == EW_UNCHANGE || alarm_b == EW_UNCHANGE)
    return;

  // Need to get info from both parts to make a decision
  // Reset change flags
  ewP->resetChange();
  cewB->resetChange();

  change = 1;
  // keep the old value of alarm
  pre_alarm = alarm;

  // As long as alarm_b is 0, reset the alarm
  if (alarm_b == 0)
    alarm = 0;
  else if (alarm_p == 0)
    alarm = 0;
  else 
    alarm = 1;

  printf("ALARM %d %d\n", pre_alarm, alarm);
}

//  make packet drop decisions
int EWPolicy::dropPacket(Packet *pkt) {
  // 1. arrival stats is measured in meter (departure and drops here)
  // 2. No penalty to response traffic!!
  // 3. need to protect existing connections

  // pass EW if there is any
  if (cewB && ewP) {
    // protecting existing connections
    //  drop requests for new connection (SYN packet)
    //    if (cewB->exFlow(pkt))
    hdr_tcp *tcph = hdr_tcp::access(pkt);
    // Protecting non-SYN packets: existing connections
    if ((tcph->flags() & TH_SYN) == 0) {
      //return(0);
    }

    // Check alarm
    detect(pkt);

    if (change) {
      // for new incoming requests:
      //   use EW measurement to adjust the rate limit (to current Rq)
      // see if the alarm should be reset
      
      if (pre_alarm) {
	if (alarm) {
	  // The rate is not right:
	  //   too low: too few connection admitted;
	  //   too high: congestion in response
	  // Adjustment is needed.
	  if (rlP)
	    rlP->adjustRate();
	} else {
	  // the current rate is ok, award the current choice
	  if (rlP)
	    rlP->adjustScore(1);
	}
      } else {
	if (alarm) {
	  if (rlP) {
	    // Start a new round
	    rlP->resetScore();
	    // Use current request rate as the rate limit
	    rlP->setRate(ewP->getRate());
	  }
	} else {
	  // the current rate is ok
	}
      }    
      
      change = 0;
    }  
  }

  // Passing rate limitor if there is any
  if (rlP) {
    // rate limiting
    return(rlP->run(1));
  };
  
  // through by default
  return(0);
}

// Enable detecting on packet incoming rate (req rate)
void EWPolicy::detectPr(int dt_inv, int db_inv) {
  ewP = new EWdetectorP;
  ewP->setLink(qsrc, qdst);
  ewP->setDt(dt_inv);
  ewP->setDb(db_inv);
}

void EWPolicy::detectPr(int dt_inv) {
  detectPr(dt_inv, dt_inv);
}

void EWPolicy::detectPr() {
  detectPr(EW_DT_INV, EW_DB_INV);
}

// Enable detecting and debugging bit rate (eg: resp rate)
void EWPolicy::detectBr(int dt_inv, int db_inv) {
  ewB = new EWdetectorB;
  ewB->init(ew_adj);
  ewB->setLink(qsrc, qdst);
  ewB->setDt(dt_inv);
  ewB->setDb(db_inv);
}

void EWPolicy::detectBr(int dt_inv) {
  detectBr(dt_inv, dt_inv);
}

void EWPolicy::detectBr() {
  detectBr(EW_DT_INV, EW_DB_INV);
}

// Rate limitor: packet rate
void EWPolicy::limitPr(double rate) {
  //printf("PR %d\n", rate);
  rlP = new TBrateLimitor(rate);
}

// Rate limitor: bit rate
void EWPolicy::limitBr(double rate) {
  //printf("BR %d\n", rate);
  rlB = new TBrateLimitor(rate);
}

// Rate limitor: packet rate
void EWPolicy::limitPr() {
  limitPr(DEFAULT_TB_RATE_P);
}

// Rate limitor: bit rate
void EWPolicy::limitBr() {
  limitBr(DEFAULT_TB_RATE_B);
}

// couple EW detector
void EWPolicy::coupleEW(EWPolicy *ewpc) {
  coupleEW(ewpc, 0);
}

// couple EW detector
void EWPolicy::coupleEW(EWPolicy *ewpc, double rate) {
  // couple the EW detector 
  cewB = ewpc->ewB;
  
  // Setup rate limitor with the default limit
  if (rate)
    limitPr(rate);
  else
    limitPr();
}
// End of EWP
