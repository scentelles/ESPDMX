const express = require('express');
const cors = require('cors');
const path = require('path');

const app = express();
const PORT = 3000;

// Middleware
app.use(cors());
app.use(express.json());

// Mock data (mutable)
let systemState = {
  activeScene: '',
  activeAmbiance: '',
  activeShow: '',
  strobeActive: false,
  smokeActive: false,
  masterBrightness: 100,
  dmxOutput: Array(512).fill(0),
  soundMode: 0,
  soundSensitivity: 5,
  audio: { volume: 0, bass: 0, mid: 0, high: 0, beat: false },
};

let mockFixtures = [
  {
    id: 'fixture-1',
    name: 'Éclairage RGB Principal',
    type: 'rgb',
    dmxAddress: 1,
    channelCount: 3,
    channels: [
      { name: 'Red', offset: 0, defaultValue: 0, type: 'color' },
      { name: 'Green', offset: 1, defaultValue: 0, type: 'color' },
      { name: 'Blue', offset: 2, defaultValue: 0, type: 'color' },
    ],
    enabled: true,
  },
  {
    id: 'fixture-2',
    name: 'Wash RGBW Scène',
    type: 'rgbw',
    dmxAddress: 4,
    channelCount: 4,
    channels: [
      { name: 'Red', offset: 0, defaultValue: 0, type: 'color' },
      { name: 'Green', offset: 1, defaultValue: 0, type: 'color' },
      { name: 'Blue', offset: 2, defaultValue: 0, type: 'color' },
      { name: 'White', offset: 3, defaultValue: 0, type: 'color' },
    ],
    enabled: true,
  },
  {
    id: 'fixture-3',
    name: 'Stroboscope',
    type: 'dimmer',
    dmxAddress: 8,
    channelCount: 1,
    channels: [
      { name: 'Dimmer', offset: 0, defaultValue: 0, type: 'dimmer' },
    ],
    enabled: true,
  },
];

let mockScenes = [
  {
    id: 'scene-1',
    name: 'Rouge Intense',
    description: 'Éclairage ambiant rouge chaleureux',
    icon: '🔴',
    fixtureValues: [
      { fixtureId: 'fixture-1', values: { Red: 255, Green: 0, Blue: 0 } },
      { fixtureId: 'fixture-2', values: { Red: 200, Green: 20, Blue: 0, White: 30 } },
    ],
  },
  {
    id: 'scene-2',
    name: 'Nuit Bleue',
    description: 'Atmosphère bleue froide',
    icon: '🔵',
    fixtureValues: [
      { fixtureId: 'fixture-1', values: { Red: 0, Green: 0, Blue: 255 } },
      { fixtureId: 'fixture-2', values: { Red: 0, Green: 50, Blue: 255, White: 10 } },
    ],
  },
  {
    id: 'scene-3',
    name: 'Énergie Verte',
    description: 'Vert énergique',
    icon: '💚',
    fixtureValues: [
      { fixtureId: 'fixture-1', values: { Red: 0, Green: 255, Blue: 50 } },
    ],
  },
  {
    id: 'scene-4',
    name: 'Magie Violette',
    description: 'Lumières violettes féeriques',
    icon: '💜',
    fixtureValues: [
      { fixtureId: 'fixture-1', values: { Red: 180, Green: 0, Blue: 255 } },
      { fixtureId: 'fixture-2', values: { Red: 150, Green: 0, Blue: 200, White: 20 } },
    ],
  },
];

let mockShows = [
  {
    id: 'show-1',
    name: 'Cycle de Couleurs',
    description: 'Alterne entre toutes les scènes couleur',
    icon: '🌈',
    loop: true,
    steps: [
      { sceneId: 'scene-1', duration: 5000, transitionTime: 2000 },
      { sceneId: 'scene-2', duration: 5000, transitionTime: 2000 },
      { sceneId: 'scene-3', duration: 5000, transitionTime: 2000 },
      { sceneId: 'scene-4', duration: 5000, transitionTime: 2000 },
    ],
  },
  {
    id: 'show-2',
    name: 'Flash Soirée',
    description: 'Changements rapides pour soirées',
    icon: '🎉',
    loop: true,
    steps: [
      { sceneId: 'scene-1', duration: 1000, transitionTime: 200 },
      { sceneId: 'scene-2', duration: 1000, transitionTime: 200 },
      { sceneId: 'scene-4', duration: 1000, transitionTime: 200 },
    ],
  },
];

// ============ READ ENDPOINTS ============

app.get('/api/scenes', (req, res) => {
  res.json({ scenes: mockScenes });
});

app.get('/api/shows', (req, res) => {
  res.json({ shows: mockShows });
});

app.get('/api/fixtures', (req, res) => {
  res.json({ fixtures: mockFixtures });
});

app.get('/api/state', (req, res) => {
  // Simulate live audio data when sound mode is active
  if (systemState.soundMode > 0) {
    systemState.audio = {
      volume: Math.floor(Math.random() * 70) + 15,
      bass: Math.floor(Math.random() * 85) + 10,
      mid: Math.floor(Math.random() * 55) + 10,
      high: Math.floor(Math.random() * 45) + 5,
      beat: Math.random() > 0.65,
    };
  }
  res.json(systemState);
});

app.get('/api/config', (req, res) => {
  res.json({
    dmxBaud: 250000,
    maxFixtures: 10,
    updateInterval: 500,
  });
});

app.get('/api/system/status', (req, res) => {
  res.json({
    uptime: Math.floor(Date.now() / 1000),
    freeMemory: 200000,
  });
});

// ============ CONTROL ENDPOINTS ============

app.post('/api/control/scene', (req, res) => {
  const { sceneId } = req.body;
  const scene = mockScenes.find((s) => s.id === sceneId);
  if (scene) {
    systemState.activeScene = sceneId;
    console.log(`✓ Scene activated: ${scene.name}`);
  }
  res.json({ status: 'ok' });
});

app.post('/api/control/color', (req, res) => {
  const { color, brightness } = req.body;
  if (color && brightness) {
    systemState.dmxOutput[0] = color[0];
    systemState.dmxOutput[1] = color[1];
    systemState.dmxOutput[2] = color[2];
    systemState.masterBrightness = brightness;
    console.log(`✓ Color set: RGB(${color.join(', ')}) Brightness: ${brightness}`);
  }
  res.json({ status: 'ok' });
});

app.post('/api/control/show', (req, res) => {
  const { showId } = req.body;
  const show = mockShows.find((s) => s.id === showId);
  if (show) {
    systemState.activeShow = showId;
    console.log(`✓ Show started: ${show.name}`);
  }
  res.json({ status: 'ok' });
});

app.post('/api/control/strobe', (req, res) => {
  const { speed } = req.body;
  systemState.strobeActive = true;
  console.log(`✓ Strobe activated: Speed ${speed}`);
  res.json({ status: 'ok' });
});

app.post('/api/control/strobe-stop', (req, res) => {
  systemState.strobeActive = false;
  console.log('✓ Strobe stopped');
  res.json({ status: 'ok' });
});

app.post('/api/control/smoke', (req, res) => {
  const { duration } = req.body;
  systemState.smokeActive = true;
  console.log(`✓ Smoke triggered: ${duration}ms`);
  setTimeout(() => {
    systemState.smokeActive = false;
  }, duration);
  res.json({ status: 'ok' });
});

app.post('/api/control/smoke-stop', (req, res) => {
  systemState.smokeActive = false;
  console.log('✓ Smoke stopped');
  res.json({ status: 'ok' });
});

app.post('/api/control/brightness', (req, res) => {
  const { brightness } = req.body;
  systemState.masterBrightness = brightness;
  console.log(`✓ Brightness set to: ${brightness}`);
  res.json({ status: 'ok' });
});

app.post('/api/control/sound', (req, res) => {
  const { mode, sensitivity } = req.body;
  if (mode !== undefined) {
    systemState.soundMode = mode;
    console.log(`✓ Sound mode: ${mode}`);
  }
  if (sensitivity !== undefined) {
    systemState.soundSensitivity = sensitivity;
    console.log(`✓ Sound sensitivity: ${sensitivity}`);
  }
  // Simulate audio data when sound mode is active
  if (systemState.soundMode > 0) {
    systemState.audio = {
      volume: Math.floor(Math.random() * 80) + 10,
      bass: Math.floor(Math.random() * 90) + 5,
      mid: Math.floor(Math.random() * 60) + 5,
      high: Math.floor(Math.random() * 50) + 5,
      beat: Math.random() > 0.6,
    };
  }
  res.json({ status: 'ok' });
});

app.post('/api/control/sound-stop', (req, res) => {
  systemState.soundMode = 0;
  systemState.audio = { volume: 0, bass: 0, mid: 0, high: 0, beat: false };
  console.log('✓ Sound mode stopped');
  res.json({ status: 'ok' });
});

// Direct DMX channel control (test sliders)
app.post('/api/control/dmx', (req, res) => {
  const { channel, value } = req.body;
  console.log(`✓ DMX channel ${channel} → ${value}`);
  res.json({ status: 'ok' });
});

// ============ SAVE/DELETE ENDPOINTS ============

// Scenes - Save
app.post('/api/scenes', (req, res) => {
  const scene = req.body;
  if (!scene.id) {
    scene.id = 'scene-' + Date.now();
  }
  const existingIdx = mockScenes.findIndex((s) => s.id === scene.id);
  if (existingIdx >= 0) {
    mockScenes[existingIdx] = scene;
    console.log(`✓ Scene updated: ${scene.name}`);
  } else {
    mockScenes.push(scene);
    console.log(`✓ Scene created: ${scene.name}`);
  }
  res.json(scene);
});

// Scenes - Delete
app.delete('/api/scenes/:id', (req, res) => {
  const { id } = req.params;
  const idx = mockScenes.findIndex((s) => s.id === id);
  if (idx >= 0) {
    const scene = mockScenes.splice(idx, 1)[0];
    console.log(`✓ Scene deleted: ${scene.name}`);
    res.json({ status: 'ok' });
  } else {
    res.status(404).json({ error: 'Scene not found' });
  }
});

// Shows - Save
app.post('/api/shows', (req, res) => {
  const show = req.body;
  if (!show.id) {
    show.id = 'show-' + Date.now();
  }
  const existingIdx = mockShows.findIndex((s) => s.id === show.id);
  if (existingIdx >= 0) {
    mockShows[existingIdx] = show;
    console.log(`✓ Show updated: ${show.name}`);
  } else {
    mockShows.push(show);
    console.log(`✓ Show created: ${show.name}`);
  }
  res.json(show);
});

// Shows - Delete
app.delete('/api/shows/:id', (req, res) => {
  const { id } = req.params;
  const idx = mockShows.findIndex((s) => s.id === id);
  if (idx >= 0) {
    const show = mockShows.splice(idx, 1)[0];
    console.log(`✓ Show deleted: ${show.name}`);
    res.json({ status: 'ok' });
  } else {
    res.status(404).json({ error: 'Show not found' });
  }
});

// Fixtures - Save
app.post('/api/fixtures', (req, res) => {
  const fixture = req.body;
  if (!fixture.id) {
    fixture.id = 'fixture-' + Date.now();
  }
  const existingIdx = mockFixtures.findIndex((f) => f.id === fixture.id);
  if (existingIdx >= 0) {
    mockFixtures[existingIdx] = fixture;
    console.log(`✓ Fixture updated: ${fixture.name}`);
  } else {
    mockFixtures.push(fixture);
    console.log(`✓ Fixture created: ${fixture.name}`);
  }
  res.json(fixture);
});

// Fixtures - Delete
app.delete('/api/fixtures/:id', (req, res) => {
  const { id } = req.params;
  const idx = mockFixtures.findIndex((f) => f.id === id);
  if (idx >= 0) {
    const fixture = mockFixtures.splice(idx, 1)[0];
    console.log(`✓ Fixture deleted: ${fixture.name}`);
    res.json({ status: 'ok' });
  } else {
    res.status(404).json({ error: 'Fixture not found' });
  }
});

// Config - Save
app.post('/api/config', (req, res) => {
  const config = req.body;
  console.log(`✓ Config saved:`, config);
  res.json(config);
});

// ============ STATIC FILES & FALLBACK ============

// Serve static files from the React app
app.use(express.static(path.join(__dirname, 'frontend', 'dist')));

// Fallback to index.html for React Router - handle all non-API routes
app.use((req, res) => {
  // If it's not an API route, serve index.html
  if (!req.path.startsWith('/api')) {
    res.sendFile(path.join(__dirname, 'frontend', 'dist', 'index.html'));
  }
});

app.listen(PORT, () => {
  console.log(`
╔════════════════════════════════════════════════════════╗
║        🎉 Serveur Mock ESP32 Démarré !                  ║
║                                                        ║
║  Local:    http://localhost:${PORT}                  ║
║  API:      http://localhost:${PORT}/api/*            ║
║                                                        ║
║  Appuyez sur Ctrl+C pour arrêter                      ║
║                                                        ║
║  ✓ CRUD complet Projecteurs, Scènes & Shows          ║
║  ✓ Tous les Contrôles : Couleur, Luminosité, Effets  ║
║  ✓ Frontend React Statique                            ║
╚════════════════════════════════════════════════════════╝
  `);
});
