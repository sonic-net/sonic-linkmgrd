# Default Route

Linkmgrd is expected to react on switch's default route changes. Orchagent will update both IPv4 and Ipv6 default route status to state db entries. If there is a valid default route, it shall be updated as `state:ok`. If there isn't a valid default route, it shall be updated as `state:na`. 

For now we only care about IPv4, but state db notification handler should be properly defined for both entries. 

## Requirements 
### Active-Standby
1. If one ToR is missing default route, it should be toggled to `standby`.
1. If one ToR is missing default route, it should remain in `standby`. It shouldn't switch to `active` even if peer ToR appears to be unhealthy. 
1. If both ToRs are missing default route, oscillation should be avoided. Mux direction should be parked on one side. 
1. dualToR should be able to recover automatically if a valid default route is added back.
1. If in manual mode, default route change shouldn't trigger mux direction change.  
1. User should have the option to turn off this feature of reacting default route changes. 

### Active-Active
1. If one ToR is missing default route, is should be toggled to `standby`, and remain in `standby`.
1. If both ToRs are missing default route, both ToRs should be in `standby`. 
1. dualToR should be able to recover automatically if a valid default route is added back.
1. If in manual mode, default route change shouldn't trigger forwarding state change.  
1. User should have the option to turn off this feature of reacting default route changes. 

## Solution One (current implementation)
### Active-Standby
* Stop sending heartbeat   
To shutdown heartbeat sending if default route is missing, is a straightforward way to fake an unhealthy status. A faked unhealthy status will help linkmgrd:
    * keep the ToR in standby if one ToR is missing default route;
    * avoid oscillation if both ToRs are missing default route (as eventually both will be `LinkProber:Wait` state).   

Link prober will still be able to receive heartbeat. So if we remove & re-add the default route on both ToRs, a state transition path can be:   

```
         0.init state   ->  1.remove on A   ->  2.remove on B     -> 3.re-add on B      -> 4.re-add on A
ToR A   active, healthy ->  standby, healthy->  active, unhealthy ->  standby, healthy  -> standby, healthy
ToR B   standby, healthy->  active, healthy ->  standby, unhealthy -> active, healthy   -> active, healthy
```

In step 3 of the transition path above, if standby side gets default route back first, we want it to switch to active. It can be a bit tricky if we have `LinkProber:Wait` previously, as a `LinkProber:Unknown` event won't be able to trigger a state transition handler. The solution is to reset `LinkProber` state to force match mux state when default route is back to `ok`. Same approach is used we linkmgrd receives `Link:Up` event. 

* Cache default route status   
To meet requirement #5, we want to cache the default route status, and when mux mode is switching between `auto` and `manual`, linkmgrd can determine if it should stop or resume heartbeat sending. 

* Avoid proactive switches  
It's obvious that, switching a ToR without default route to `active` is meaningless. Though eventually, due to suspending of heartbeat, the other ToR will take over the traffic, we will expect 300 ms packet loss in between. Thus, we will explicitly avoid switching to `active` if a ToR is missing default route.   
The state transition path above will hence be updated at step #2, like below:

```
         0.init state   ->  1.remove on A   ->  2.remove on B     -> 3.re-add on B      -> 4.re-add on A
ToR A   active, healthy ->  standby, healthy->  standby, unhealthy-> standby, healthy  -> standby, healthy
ToR B   standby, healthy->  active, healthy ->  active, unhealthy -> active, healthy   -> active, healthy
```

* Add linkmgrd command line option 
We will add command line option `--default_route, -d` for linkmgrd process for user to enable this feature by choice.

### Active-Active
* Stop sending heartbeat  
Same logic as Active-Standby state machine, suspending heartbeat can fake an unhealthy state so ToR can stay in `standby` state. But there is no need to reset `LinkProber` state in this case. 

* Cache default route status
We still need to cache default route state. When mux mode is updated, we can resume or stop heartbeat accordingly. 

* There is no need to avoid proactive switchovers in Active-Active state machine, as two ToRs are expected to be more independent. One ToR entering unhealthy state won't force the other one to switch to active. 

* Add linkmgrd command line option We will add command line option --default_route, -d for linkmgrd process for user to enable this feature by choice.

## Solution Two 

Intergrate default route status into `LinkManagerStateMachine`.   
Currently the state transition depends on 3 components (`LinkProber`, `MuxState`, `LinkState`), adding one more dimension (`DefaultRoute`) is quite more effort compared to solution one. But the code will be more readable, and easier to maintain. Moving forward to active-active dualToR, we can revisit this part of requirement and take this solution into consideration. 

## Test

When missing valid default route and this feature is enabled, some dualtor_io tests are expected to fail. We consider this as a testing environment failure, not an image failure.  
