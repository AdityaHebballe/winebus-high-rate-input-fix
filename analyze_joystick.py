#!/usr/bin/env python3
import csv,sys
if len(sys.argv)!=3:raise SystemExit(f"usage: {sys.argv[0]} producer.csv winmm.csv")
events=[{**r,"realtime_us":int(r["realtime_us"])} for r in csv.DictReader(open(sys.argv[1]))]
states=[{k:int(v) for k,v in r.items()} for r in csv.DictReader(open(sys.argv[2]))]
ids={s["id"] for s in states if max(abs(s[a]-32767) for a in ("x","y","z","r","u"))>1000}
if len(ids)!=1:raise SystemExit(f"FAIL: expected one moving generic joystick, found {sorted(ids)}")
joy=ids.pop();print(f"generic joystick WinMM id={joy}")
after=0
for name,want in (("button_down",True),("button_up",False)):
 e=next(x for x in events if x["event"]==name);s=next((x for x in states if x["id"]==joy and x["realtime_us"]>=max(e["realtime_us"],after) and bool(x["buttons"]&1)==want),None)
 if not s:raise SystemExit(f"FAIL: {name} missing")
 print(f"PASS {name} {(s['realtime_us']-e['realtime_us'])/1000:.3f} ms")
 after=s["realtime_us"]+1
e=next(x for x in events if x["event"]=="neutral");s=next((x for x in states if x["id"]==joy and x["realtime_us"]>=e["realtime_us"] and max(abs(x[a]-32767) for a in ("x","y","z","r","u"))<=1),None)
if not s:raise SystemExit("FAIL: neutral missing")
print(f"PASS generic joystick neutral {(s['realtime_us']-e['realtime_us'])/1000:.3f} ms")
print("ALL GENERIC JOYSTICK CHECKS PASSED")
