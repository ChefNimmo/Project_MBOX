# Queued USB diagnostic and offline validation

Date: 2026-11-04 (Australia/Sydney). Follow-up to CUSTOM-TRANSPORT-01.md.
User constraint: develop on the current PC; do not change boot security. No signing account or
separate test PC is currently available. The unsigned custom driver remains unloaded.

## Changes

- Replaced one-request-per-direction admission with four pending requests per direction. A shared,
  pure admission-state helper rejects overflow, invalid states, counter underflow and transfers
  during rate changes. Driver transitions use atomic compare/exchange; tests exhaust all 25 legal
  combinations of OUT/IN request counts. Atomic race behaviour is not established by these tests.
- Added KMDF EvtIoStop cancellation of sent requests with a request lifetime reference. Added
  D0Entry invalidation of cached sample rate, requiring SET/GET verification after power transitions.
  These kernel callbacks have compiled and passed source analysis but have not run on hardware.
- Added a silent streaming controller with four eight-frame buffers per direction, fractional
  44.1 kHz packet-clock continuity, per-packet validation, sample counters and capture peak values.
  It cancels all pending requests before draining them, then releases buffers/events. An idle
  watchdog stops the diagnostic after fifty 100-ms waits without completions. Draining waits for
  ownership to return; it does not free memory while a lower driver might still use it.
- Added deterministic simulation using the actual client scheduler with a fake backend. It checks
  known positive/negative stereo samples and varies completion order to stress slot ownership.
- The static analyser identified an event-cleanup path it could not prove initialised. Event
  ownership now stays in a separate scheduler-owned array with an explicit successful-creation
  count; cleanup closes only those handles. No source diagnostic suppression was added.

## Results

`native/build.cmd` completed with exit code 0, `/W4 /WX` and source static analysis enabled for
the kernel transport and streaming controller. SDK/WDK header implementations are marked external.
Core packet/conversion tests, admission tests and Microsoft InfVerif all passed.

The client scheduler passed these **simulated** checks at both 44100 and 48000 Hz:

| Case | Result |
|---|---|
| Ten virtual minutes, silent OUT and known synthetic stereo IN | 75,000 transfers completed per direction; exact rate x 600 sample-frame totals; expected signed channel peaks |
| Submission failure on third request | Both earlier requests cancelled and drained |
| Completion failure | Seven other outstanding requests cancelled and drained |
| Requested cancellation after twenty completions | Seven outstanding requests cancelled and drained |
| Capture packet length 305 (beyond 304-byte endpoint maximum) | Rejected; remaining requests drained |
| Short output structure | Rejected; remaining requests drained |
| Overall USB error status | Rejected; remaining requests drained |
| Wrong capture packet offset | Rejected; remaining requests drained |
| No completions / idle watchdog | All eight pending requests cancelled and drained |

For all 18 cases, fake-backend admission state and scheduler pending counts returned to zero.
Healthy runs never exceeded eight total pending requests. The simulation runs faster than real time;
it does not represent twenty minutes of actual USB/audio operation or real-time scheduling stress.

The real-backend diagnostic was invoked once while the custom interface was absent. It reported:
`Custom research transport unavailable. No hardware operation performed.`
PnP still reported the original Mbox started under Ploytec's `oem51.inf` package.

## Limits and next work

No new kernel code has been loaded, and no custom USB transfer has been sent to the Mbox. Tests do
not execute the WDF submission/completion/cancellation callbacks, test pool leaks, exercise power
transitions, prove USB sample cadence, or measure latency/dropouts. Kernel cancellation, target
recovery, descriptor assumptions and the historical synchronous playback quirk still need validation.
The driver currently allocates request-associated transfer storage per IOCTL; preallocation and
resource-lifetime verification should be considered before low-latency production use.

ASIO/WASAPI integration and a variable-packet-to-host-block audio adapter are still missing. This
diagnostic outputs silence only, so the user cannot select it as an audio interface yet. The existing
Ploytec demo beep remains. S/PDIF is deferred.

## References

- [Explicit isochronous packet sizes and URBs](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/transfer-data-to-isochronous-endpoints)
- [Power-managed queue stop callbacks](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdfio/nc-wdfio-evt_wdf_io_queue_io_stop)
- [Cancelling sent requests](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdfrequest/nf-wdfrequest-wdfrequestcancelsentrequest)
- [Request memory life cycle](https://learn.microsoft.com/en-us/windows-hardware/drivers/wdf/memory-buffer-life-cycle)
