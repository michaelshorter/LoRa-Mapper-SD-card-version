import java.util.ArrayList;
import java.io.FileWriter;
import java.io.IOException;

// Map image
PImage mapImg;

// Map boundaries
float latMin = 53.377366;
float latMax = 53.389229;
float lonMin = -2.949590;
float lonMax = -2.916461;

// Debug panel
ArrayList<String> debugLines = new ArrayList<String>();
int maxLines   = 20;
int panelX     = 800;
int panelWidth = 200;

// Stored plot points
ArrayList<Float>   dotX     = new ArrayList<Float>();
ArrayList<Float>   dotY     = new ArrayList<Float>();
ArrayList<Integer> dotColor = new ArrayList<Integer>();

// Data file — drop your SD card log.csv into the sketch data/ folder
// and update this filename to match
String dataFile = "log.csv";

void setup() {
  size(1000, 600);

  mapImg = loadImage("map.png");
  panelX = mapImg.width;
  println("Map size: " + mapImg.width + " x " + mapImg.height);
  surface.setSize(mapImg.width + panelWidth, mapImg.height);

  textFont(createFont("Arial", 12));

  // Load dots from file automatically on startup
  loadDots();

  debugLines.add("Loaded " + dotX.size() + " points.");
  debugLines.add("Press L to reload file.");
  debugLines.add("Press C to clear all.");
  debugLines.add("Press SPACE to add test dot.");
}

void draw() {
  // Draw map
  image(mapImg, 0, 0);

  // Redraw all stored dots
  noStroke();
  for (int i = 0; i < dotX.size(); i++) {
    fill(dotColor.get(i));
    ellipse(dotX.get(i), dotY.get(i), 8, 8);
  }

  // Draw debug panel
  fill(0, 150);
  rect(panelX, 0, panelWidth, height);
  fill(255);
  textSize(12);
  for (int i = 0; i < debugLines.size(); i++) {
    text(debugLines.get(i), panelX + 10, 20 + i * 15);
  }
}

// Load all dots from CSV file
void loadDots() {
  dotX.clear();
  dotY.clear();
  dotColor.clear();

  File f = new File(sketchPath("data/" + dataFile));
  if (!f.exists()) {
    println("No data file found at: " + sketchPath("data/" + dataFile));
    debugLines.add("No file: data/" + dataFile);
    return;
  }

  String[] lines = loadStrings("data/" + dataFile);
  if (lines == null) return;

  int loaded = 0;
  for (String line : lines) {
    line = line.trim();
    if (line.length() == 0) continue;

    // Handle both tab and comma separated files
    String[] parts;
    if (line.contains("\t")) {
      parts = split(line, '\t');
    } else {
      parts = split(line, ',');
    }

    // Handle 3-column format: Result,Lat,Lon
    if (parts.length == 3) {
      String result = parts[0];
      String rawLat = parts[1];
      String rawLon = parts[2];

      // Skip header row and no-fix entries
      if (rawLat.equals("Latitude") || rawLat.equals("GPS_NOT_FIXED")) continue;

      float lat = float(rawLat);
      float lon = float(rawLon);

      float x = map(lon, lonMin, lonMax, 0, panelX);
      float y = map(lat, latMax, latMin, 0, height);

      int c;
      if (result.equals("Direct"))      c = color(0, 255, 0);
      else if (result.equals("Failed")) c = color(255, 0, 0);
      else                              c = color(255);

      dotX.add(x);
      dotY.add(y);
      dotColor.add(c);
      loaded++;
    }
  }
  println("Loaded " + loaded + " dots from " + dataFile);
}

void keyPressed() {
  // Spacebar — add a random test dot
  if (key == ' ') {
    String[] types = {"Direct", "Failed"};
    String result  = types[(int)random(2)];
    float randLat  = random(latMin, latMax);
    float randLon  = random(lonMin, lonMax);
    float x = map(randLon, lonMin, lonMax, 0, panelX);
    float y = map(randLat, latMax, latMin, 0, height);

    int c = result.equals("Direct") ? color(0, 255, 0) : color(255, 0, 0);
    dotX.add(x);
    dotY.add(y);
    dotColor.add(c);

    debugLines.add("TEST " + result + " " + nf(randLat, 1, 4) + ", " + nf(randLon, 1, 4));
    if (debugLines.size() > maxLines) debugLines.remove(0);
  }

  // L — reload the data file
  if (key == 'l' || key == 'L') {
    loadDots();
    debugLines.add("Reloaded: " + dotX.size() + " points.");
    if (debugLines.size() > maxLines) debugLines.remove(0);
  }

  // C — clear everything
  if (key == 'c' || key == 'C') {
    dotX.clear();
    dotY.clear();
    dotColor.clear();
    debugLines.add("Cleared.");
    if (debugLines.size() > maxLines) debugLines.remove(0);
  }
}
