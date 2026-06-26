Test Case 1: Verify that when the device is powered on and connected to the network, the green LED stays solid and the blue MQTT light blinks continuously.

Test Case 2: Verify that disconnecting the company network successfully changes the machine's state from Locked to Unlocked.

Test Case 3: Verify that all network-dependent functions of the interlocking device operate correctly under normal network conditions.

Test Case 4: Verify that if the server-side network is disconnected while the machine is Locked, the machine automatically changes to an Unlocked state after a predetermined timeout period.

Test Case 5: Verify that the machine defaults to an Unlocked state if the interlocking device is completely disconnected from the network.

Test Case 6: Verify that if the power supply is cut and then restored while the machine is Locked, the machine safely reboots into the expected Unlocked state.

Test Case 7: Verify that if the network is lost when the machine has 10 to 20 seconds of idle time remaining, and the machine then transitions to a productive state, it correctly displays an Unlocked state once the network reconnects.

Test Case 8:Verify that when the machine is Locked, electrical continuity shifts to the Normally Open (NO) position, and when it is Unlocked, continuity shifts to the Normally Closed (NC) position.
