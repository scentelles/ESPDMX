# DMXESP - User Guide

## Overview

DMXESP is a professional-grade DMX lighting control system designed for event rentals, parties, weddings, and performances. The system provides both a simple user interface for operators and a comprehensive admin panel for technicians.

## User Interface

### Main Features

#### 1. Master Brightness
Located at the top of the user page, this slider controls the overall brightness of all connected lighting fixtures (0-100%).

**Usage:**
- Slowly fade in/out lights during events
- Adjust overall intensity based on venue/time of day
- Affects all active scenes and fixtures

#### 2. Color Scenes 🎨
Pre-configured single colors or color combinations optimized for different event types.

**Default Scenes:**
- **Warm White**: Classic ambient lighting (255, 180, 100)
- **Cool White**: Professional/corporate look (220, 220, 255)
- **Red**: Energy and passion (255, 0, 0)
- **Blue**: Calm and sophistication (0, 100, 255)
- **Green**: Fresh and natural look (0, 255, 100)
- **Purple**: Romantic or mysterious ambiance (200, 0, 255)
- **Pink**: Warm and inviting (255, 100, 180)

**How to Use:**
1. Browse available scenes in the "Color Scenes" section
2. Click any scene to instantly apply it
3. The selected scene will highlight in purple
4. Adjust master brightness to fine-tune the look

#### 3. Ambiances 🌙
Pre-set combinations of scenes designed for specific event types or moods.

**Typical Ambiances:**
- Dinner: Warm, romantic lighting
- Party: Vibrant, dynamic colors
- Lounge: Soft, relaxing tones
- Presentation: Clean, professional whites

**How to Use:**
1. Select an ambiance to activate its color scheme
2. The ambiance may activate multiple scenes sequentially
3. Brightness adjusts automatically based on ambiance design
4. Can be overridden by selecting individual scenes

#### 4. Dynamic Shows 🎆
Pre-programmed animated lighting sequences that cycle through colors, patterns, and effects.

**Common Shows:**
- **Rainbow**: Smooth transition through spectrum colors
- **Pulse**: Rhythmic brightness increases and decreases
- **Chase**: Colors move across the venue
- **Strobe**: Intense flashing effect (use with caution)
- **Party Mode**: High-energy multi-color mix

**How to Use:**
1. Browse shows in the "Dynamic Shows" section
2. Click to start a show
3. Show will loop for its programmed duration
4. Green indicator appears while running
5. Click again or select another scene to stop

#### 5. Quick Effects

##### STROBE (Red Button)
Intense flashing effect for high-energy moments.

**Safety:**
⚠️ Can cause discomfort or health issues for sensitive individuals
⚠️ Use responsibly - warn attendees when engaging strobe

**How to Use:**
1. Press and hold STROBE button for flashing effect
2. Strobe runs while button is held
3. Release to stop

**Guidelines:**
- Max 2-3 seconds per use
- Provide 30+ second breaks between uses
- Display warning signs if used at events with health-conscious attendees
- Use in moderation for best effect

##### SMOKE (Red Button)
Triggers fog/smoke machine (if connected).

**How to Use:**
1. Press SMOKE button to trigger machine
2. Smoke machine activates for 3 seconds
3. Wait for machine to cool before next use
4. Use to enhance lighting effects and atmosphere

**Tips:**
- Combine with purple/blue lighting for mysterious effect
- Use during show transitions for drama
- Allow machine heat to dissipate (2-3 min between uses)
- Ensure adequate ventilation

### Operating Tips

#### For Best Results:
1. **Start Simple**: Begin with single color scenes
2. **Layer Effects**: 
   - Start with static color scene (warmth)
   - Add ambiance if appropriate
   - Overlay dynamic show for energy
3. **Lighting Levels**:
   - Cocktail hour: 60-80% brightness
   - Dinner: 40-60% brightness
   - Dancing: 80-100% brightness
   - Transitions: Fade gradually (not instant changes)
4. **Timing**: 
   - Allow 2-3 seconds between color changes
   - Let shows complete at least one cycle before changing
   - Use effects sparingly for maximum impact

#### Scene Selection Guide:

**Ceremony/Dinner:**
- Warm White or Pink for elegant feel
- 50-70% brightness for intimate lighting
- Static scenes (no shows) for digestion

**Cocktail Hour:**
- Purple or Blue for sophistication  
- 70% brightness to highlight the space
- Can use gentle shows like Pulse

**Dancing/Reception:**
- Dynamic shows (Rainbow, Chase, Party)
- 80-100% brightness
- Mix colors every 2-3 minutes
- Use strobe and smoke strategically

**Photo Opportunities:**
- Warm White or Cool White
- 80% brightness for proper exposure
- Static scenes for consistent lighting

### Troubleshooting (User)

**Lights not responding:**
- Check if system is powered on
- Try selecting a different scene
- Reload web page (F5)

**Brightness changes are slow:**
- This is intentional for smooth fades
- Use preset scenes for instant color changes

**Effects not available:**
- System may need to cool (5 minute wait between heavy smoke use)
- Some effects require proper fixture configuration

## Admin Panel

Access the admin panel by clicking the ⚙️ gear icon (bottom right) and entering your PIN.

### Admin Tabs

#### Fixtures
Manage all connected DMX lighting equipment.

**Add Fixture:**
1. Click "+ Add Fixture"
2. Enter fixture name (e.g., "Main LED Bar")
3. Select fixture type:
   - **Dimmer**: Single channel brightness
   - **Color**: Color-changing lightDMX 
   - **RGB**: Red, Green, Blue channels
   - **RGBW**: RGB with White channel
   - **Moving Head**: Pan, tilt, and color
4. Set DMX address (1-512)
5. Enter number of channels used
6. Click Save

**Edit Fixture:**
1. Click "Edit" button on fixture
2. Modify any settings
3. Click Save

**Delete Fixture:**
1. Click "Delete" button (trash icon)
2. Confirm deletion
3. Fixture removed from system

**Important:** 
- DMX addresses must not overlap
- RGB fixture = 3 channels minimum
- Ensure correct channel count or fixture won't work

#### Scenes
Create, edit, and manage color scenes.

**Create Scene:**
1. Click "+ Add Scene"
2. Enter scene name (e.g., "Royal Blue")
3. Set color using color picker
4. Adjust brightness (0-100%)
5. Choose icon/emoji
6. Enter description
7. Click Save

**Edit Scene:**
- Click "Edit" to modify
- Can't change ID, but name and color are editable
- Changes apply immediately

**Delete Scene:**
- Click "Delete" to remove
- If scene is in use, it will stop working for users

#### Shows
Create custom dynamic lighting sequences.

**Create Show:**
1. Click "+ Add Show"
2. Enter show name (e.g., "Blue Pulse")
3. Set duration in milliseconds (e.g., 30000 = 30 seconds)
4. Choose icon
5. Enter description (what happens during show)
6. Click Save

**Show Building Tips:**
- Duration should be 15-60 seconds for most shows
- Shorter shows are more responsive to user control
- Longer shows better for background ambiance
- Test duration before deploying to events

#### Settings
System-wide configuration.

**WiFi Configuration:**
1. Enter WiFi SSID (network name)
2. Enter WiFi password
3. Click Save Settings
4. System will reboot and reconnect

**Admin PIN:**
1. Enter new PIN (numbers only)
2. Click Save Settings
3. New PIN required for next admin access

**System Status:**
- Uptime: How long device has been running
- Free Memory: Available RAM (should stay above 50KB)
- If memory is low, device may reboot automatically

**Reboot System:**
1. Click "Reboot System" button
2. Device will restart (takes 30 seconds)
3. Web interface will reconnect automatically

### Maintenance

**Regular Tasks:**
- Weekly: Check WiFi connection, test all scenes
- Monthly: Update scenes based on seasonal events
- Quarterly: Update/verify fixture configuration
- Yearly: Review and backup all settings

**Performance Monitoring:**
- Watch "Free Memory" in Settings tab
- If consistent drop, may need to reboot
- Clear unused scenes/shows periodically
- Restart device weekly for optimal performance

## Specifications

### System
- **Controller**: ESP32 microcontroller
- **Network**: WiFi 802.11b/g/n (2.4GHz)
- **Storage**: 4MB SPIFFS for configuration
- **Power**: 5V @ 2A minimum

### DMX Output
- **Standard**: DMX512-A (USITT)
- **Interface**: RS-485 via UART
- **Baud Rate**: 250,000 bps (standard)
- **Max Channels**: 512 (full universe)
- **Max Fixtures**: 64 simultaneous
- **Update Rate**: 44Hz (22.7ms per frame)

### Web Interface
- **Responsive**: Works on desktop, tablet, mobile
- **Browsers**: Chrome, Firefox, Safari, Edge (modern versions)
- **Protocol**: HTTP (not HTTPS in basic version)
- **Port**: 80

### Lighting Effects
- **Strobe**: 1-10 Hz adjustable
- **Master Brightness**: 0-255 (0-100%)
- **Scene Transitions**: Adjustable fade time
- **Show Duration**: 1-120+ seconds

---

**For technical support or feature requests, contact your system administrator.**

**Safety First: Always test lighting changes with attendees present. Never use strobe without warning vulnerable populations.**
