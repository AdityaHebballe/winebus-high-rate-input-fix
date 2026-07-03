#!/usr/bin/env python3
import csv, sys

if len(sys.argv) != 4:
    raise SystemExit(f"usage: {sys.argv[0]} producer.csv states.csv rumble.csv")
events=[{**r,"realtime_us":int(r["realtime_us"]),"pad":int(r["pad"]),"value":int(r["value"])} for r in csv.DictReader(open(sys.argv[1]))]
states=[{k:int(v) for k,v in r.items()} for r in csv.DictReader(open(sys.argv[2]))]
rumble=[r for r in csv.DictReader(open(sys.argv[3]))]
fail=[]

def first_after(t,pred,limit=100000):
    return next((s for s in states if t<=s["realtime_us"]<=t+limit and s["connected"] and pred(s)),None)

# Identify slots from unique initial markers.
markers={1:0x1000,2:0x2000}; slots={}
for pad,bit in markers.items():
    ev=next(e for e in events if e["pad"]==pad and e["event"] in ("a","b") and e["value"]==1)
    hit=first_after(ev["realtime_us"],lambda s,b=bit:s["buttons"]&b,250000)
    if hit: slots[pad]=hit["slot"]
    else: fail.append(f"pad {pad}: identification marker missing")
print("slot mapping:",slots)

checks={"a":0x1000,"b":0x2000,"x":0x4000,"y":0x8000,"dpad_right":0x0008,"dpad_up":0x0001,"reconnect_a":0x1000}
latencies=[]
for e in events:
    if e["event"] not in checks or e["pad"] not in slots: continue
    bit=checks[e["event"]]; slot=slots[e["pad"]]; want=e["value"]!=0
    hit=first_after(e["realtime_us"],lambda s,b=bit,w=want,sl=slot:s["slot"]==sl and bool(s["buttons"]&b)==w,250000)
    if hit:
        delay=(hit["realtime_us"]-e["realtime_us"])/1000;latencies.append(delay);print(f"PASS pad{e['pad']} {e['event']}={int(want)} {delay:.3f} ms")
    else: fail.append(f"pad{e['pad']} {e['event']}={int(want)} missing")

for name,field in (("lt","lt"),("rt","rt")):
    for e in (x for x in events if x["event"]==name):
        if e["pad"] not in slots: continue
        slot=slots[e["pad"]];want=e["value"]
        hit=first_after(e["realtime_us"],lambda s,f=field,w=want,sl=slot:s["slot"]==sl and s[f]==w,250000)
        if hit: print(f"PASS pad{e['pad']} {name}={want} {(hit['realtime_us']-e['realtime_us'])/1000:.3f} ms")
        else: fail.append(f"pad{e['pad']} {name}={want} missing")

if len(set(slots.values()))!=2: fail.append("two controllers did not receive distinct XInput slots")
removed=next((e for e in events if e["event"]=="removed" and e["pad"]==1),None)
if removed and 1 in slots and any(not s["connected"] and s["slot"]==slots[1] and s["realtime_us"]>=removed["realtime_us"] for s in states): print("PASS hot-unplug observed")
else: fail.append("hot-unplug transition missing")

successful={int(r["slot"]) for r in rumble if r["action"]=="start" and int(r["result"])==0}
ffpads={e["pad"] for e in events if e["event"] in ("ff_upload","ff_play")}
if len(successful)>=2: print("PASS XInput rumble accepted on two slots")
else: fail.append("XInput rumble was not accepted on two slots")
if ffpads=={1,2}: print("PASS Linux received force-feedback for both pads")
else: fail.append(f"force-feedback reached pads {sorted(ffpads)}, expected [1, 2]")

if latencies: print(f"ordered edge latency: max={max(latencies):.3f} ms, checks={len(latencies)}")
if fail:
    print("FAIL:");print("\n".join("- "+x for x in fail));sys.exit(1)
print("ALL REGRESSION CHECKS PASSED")
