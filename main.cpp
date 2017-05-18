#include <GL/glut.h>
#include <fstream>
#include <cmath>
#include <vector>
#include <list>
#include <algorithm>
#include <iostream>

#define ImageW 400
#define ImageH 400

using namespace std;

float framebuffer[ImageH][ImageW][3];

struct color
{
	float r, g, b;		// Color (R,G,B values)
	color(float rr, float gg, float bb)
		: r(rr), g(gg), b(bb) {}
};

const color white(1.0f, 1.0f, 1.0f);
const color red(1.0f, 0.0f, 0.0f);
const color green(0.0f, 1.0f, 0.0f);
const color blue(0.0f, 0.0f, 1.0f);
const color yellow(1.0f, 1.0f, 0.0f);
const color cyan(0.0f, 1.0f, 1.0f);
const color magenta(1.0f, 0.0f, 1.0f);
const color orange(1.0f, 0.5f, 0.0f);
const color brown(0.5f, 0.3f, 0.0f);
const color gray(0.5f, 0.5f, 0.5f);

// clamp x between min and max
template<typename T>
T clamp(T x, T min, T max)
{
	if (x < min) return min;
	if (x > max) return max;
	return x;
}

// Draws the scene
void drawit(void)
{
	glDrawPixels(ImageW, ImageH, GL_RGB, GL_FLOAT, framebuffer);
	glFlush();
}

// Clears framebuffer to black
void clearFramebuffer()
{
	int i, j;

	for (i = 0; i<ImageH; i++) {
		for (j = 0; j<ImageW; j++) {
			framebuffer[i][j][0] = 0.0;
			framebuffer[i][j][1] = 0.0;
			framebuffer[i][j][2] = 0.0;
		}
	}
}

// Sets pixel x,y to the color RGB
void setFramebuffer(int x, int y, color c)
{
	// changes the origin from the lower-left corner to the upper-left corner
	y = ImageH - 1 - y;
	framebuffer[y][x][0] = clamp(c.r, 0.0f, 1.0f);
	framebuffer[y][x][1] = clamp(c.g, 0.0f, 1.0f);
	framebuffer[y][x][2] = clamp(c.b, 0.0f, 1.0f);
}

struct point
{
	int x;
	int y;
	point(int xx, int yy)
		: x(xx), y(yy) {}
};

struct line
{
	int minX;
	int minY;
	int maxX;
	int maxY;
	double dxdy;
	line(point p1, point p2);
};

line::line(point p1, point p2)
{
	minX = min(p1.x, p2.x);
	maxX = max(p1.x, p2.x);
	minY = min(p1.y, p2.y);
	maxY = max(p1.y, p2.y);
	if (p1.y != p2.y)
	{
		dxdy = (double)(p2.x - p1.x) / (p2.y - p1.y);
	}
}

class polygon
{
	vector<point> points;
	vector<line> lines;

	color c;

	int minX;
	int maxX;
	int minY;
	int maxY;

	void addLine(int i1, int i2);
public:
	polygon(color cc)
		: c(cc) {}
	polygon()
		: c(white) {}
	bool addPoint(point p);
	bool addLastPoint(point p);
	void close();
	void scanConvert();

	friend polygon clipRight(polygon& p, int bound);
	friend polygon clipBottom(polygon& p, int bound);
	friend polygon clipLeft(polygon& p, int bound);
	friend polygon clipTop(polygon& p, int bound);
};

bool polygon::addPoint(point p)
{
	points.push_back(p);
	if (points.size() > 1)
	{
		addLine(points.size() - 2, points.size() - 1);
		if (p.x > maxX) maxX = p.x;
		if (p.x < minX) minX = p.x;
		if (p.y > maxY) maxY = p.y;
		if (p.y < minY) minY = p.y;
	}
	else
	{
		maxX = p.x;
		minX = p.x;
		maxY = p.y;
		minY = p.y;
	}
	return points.size() < 10;
}

bool polygon::addLastPoint(point p)
{
	if (points.size() > 1)
	{
		points.push_back(p);
		addLine(points.size() - 2, points.size() - 1);
		addLine(points.size() - 1, 0);
		if (p.x > maxX) maxX = p.x;
		if (p.x < minX) minX = p.x;
		if (p.y > maxY) maxY = p.y;
		if (p.y < minY) minY = p.y;
		return true;
	}
	return false;
}

void polygon::close()
{
	addLine(points.size() - 1, 0);
}

void polygon::addLine(int i1, int i2)
{
	lines.push_back(line(points[i1], points[i2]));
}

void polygon::scanConvert()
{
	// Create sorted edge table
	vector< list<line> > sortedEdges(maxY - minY + 1);
	for (line& l : lines)
	{
		sortedEdges[l.minY - minY].push_back(l);
	}

	// Initialize active edge list
	list< pair<line, double> > activeEdges;

	// Start scan lines
	for (int y = minY; y < maxY; ++y)
	{
		// Update active edge list
		for (line l : sortedEdges[y - minY])
		{
			double startX = l.dxdy > 0 ? l.minX : l.maxX;
			activeEdges.push_back(pair<line, double>(l, startX));
		}
		vector<double> sortedXs;
		for (auto iter = activeEdges.begin(); iter != activeEdges.end();)
		{
			line& l = (*iter).first;
			double& edgeX = (*iter).second;
			if (y == l.maxY)
			{
				iter = activeEdges.erase(iter);
			}
			else
			{
				edgeX += l.dxdy;
				sortedXs.push_back(edgeX);
				++iter;
			}
		}

		sort(sortedXs.begin(), sortedXs.end());
		
		// Start Drawing
		int edgesCrossed = 0;
		for (int x = minX; x < maxX; ++x)
		{
			while (x >= sortedXs[edgesCrossed])
			{
				++edgesCrossed;
				if (edgesCrossed >= sortedXs.size()) break;
			}
			if (edgesCrossed == sortedXs.size())
			{
				break;
			}
			if (edgesCrossed % 2 == 1)
			{
				setFramebuffer(x, y, c);
			}
		}
	}
}

polygon clipRight(polygon& p, int bound)
{
	polygon ret(p.c);

	for (int i = 0; i < p.points.size(); ++i)
	{
		point prev = p.points[(i + p.points.size() - 1) % p.points.size()];
		point current = p.points[i];

		// line entirely inside
		if (prev.x < bound && current.x < bound)
		{
			ret.addPoint(current);
		}
		// line staring inside, going out
		else if (prev.x < bound)
		{
			float m = (float)(current.y - prev.y) / (current.x - prev.x);
			ret.addPoint(point(bound, m * (bound - current.x) + current.y));
		}
		// line starting outside, going in
		else if (current.x < bound)
		{
			float m = (float)(current.y - prev.y) / (current.x - prev.x);
			ret.addPoint(point(bound, m * (bound - current.x) + current.y));
			ret.addPoint(current);
		}
	}

	if (ret.points.size() > 0)
	{
		ret.close();
	}
	return ret;
}

polygon clipBottom(polygon& p, int bound)
{
	polygon ret(p.c);

	for (int i = 0; i < p.points.size(); ++i)
	{
		point prev = p.points[(i + p.points.size() - 1) % p.points.size()];
		point current = p.points[i];

		// line entirely inside
		if (prev.y < bound && current.y < bound)
		{
			ret.addPoint(current);
		}
		// line staring inside, going out
		else if (prev.y < bound)
		{
			float m = (float)(current.x - prev.x) / (current.y - prev.y);
			ret.addPoint(point(m * (bound - current.y) + current.x, bound));
		}
		// line starting outside, going in
		else if (current.y < bound)
		{
			float m = (float)(current.x - prev.x) / (current.y - prev.y);
			ret.addPoint(point(m * (bound - current.y) + current.x, bound));
			ret.addPoint(current);
		}
	}
	if (ret.points.size() > 0)
	{
		ret.close();
	}
	return ret;
}

polygon clipLeft(polygon& p, int bound)
{
	polygon ret(p.c);

	for (int i = 0; i < p.points.size(); ++i)
	{
		point prev = p.points[(i + p.points.size() - 1) % p.points.size()];
		point current = p.points[i];

		// line entirely inside
		if (prev.x > bound && current.x > bound)
		{
			ret.addPoint(current);
		}
		// line staring inside, going out
		else if (prev.x > bound)
		{
			float m = (float)(current.y - prev.y) / (current.x - prev.x);
			ret.addPoint(point(bound, m * (bound - current.x) + current.y));
		}
		// line starting outside, going in
		else if (current.x > bound)
		{
			float m = (float)(current.y - prev.y) / (current.x - prev.x);
			ret.addPoint(point(bound, m * (bound - current.x) + current.y));
			ret.addPoint(current);
		}
	}

	if (ret.points.size() > 0)
	{
		ret.close();
	}
	return ret;
}

polygon clipTop(polygon& p, int bound)
{
	polygon ret(p.c);

	for (int i = 0; i < p.points.size(); ++i)
	{
		point prev = p.points[(i + p.points.size() - 1) % p.points.size()];
		point current = p.points[i];

		// line entirely inside
		if (prev.y > bound && current.y > bound)
		{
			ret.addPoint(current);
		}
		// line staring inside, going out
		else if (prev.y > bound)
		{
			float m = (float)(current.x - prev.x) / (current.y - prev.y);
			ret.addPoint(point(m * (bound - current.y) + current.x, bound));
		}
		// line starting outside, going in
		else if (current.y > bound)
		{
			float m = (float)(current.x - prev.x) / (current.y - prev.y);
			ret.addPoint(point(m * (bound - current.y) + current.x, bound));
			ret.addPoint(current);
		}
	}

	if (ret.points.size() > 0)
	{
		ret.close();
	}
	return ret;
}

polygon clip(polygon& p, int minX, int maxX, int minY, int maxY)
{
	polygon pr = clipRight(p, maxX);
	polygon pb = clipBottom(pr, maxY);
	polygon pl = clipLeft(pb, minX);
	polygon pfin = clipTop(pl, minY);
	return pfin;
}

vector<polygon> originalPolygons;
vector<polygon> clippedPolygons;
int colorNo = 0;
color colors[10] = {
	white, red, blue,
	green, yellow, cyan,
	magenta, orange, brown,
	gray
};
polygon current(colors[colorNo]);

bool clipping = false;
bool clippedYet = false;

point clipCorner1(0, 0);
point clipCorner2(0, 0);

void onKeyPressed(unsigned char key, int x, int y)
{
	if (key == 'c')
	{
		clipping = true;
		clippedPolygons = vector<polygon>(originalPolygons.size());
	}
}

void onClick(int button, int state, int x, int y)
{
	if (!clipping)
	{
		if (state == GLUT_DOWN)
		{
			if (button == GLUT_LEFT_BUTTON)
			{
				if (!current.addPoint(point(x, y)))
				{
					current.close();
					originalPolygons.push_back(current);
					++colorNo;
					current = polygon(colors[colorNo]);
					glutPostRedisplay();
				}
			}
			else if (button == GLUT_RIGHT_BUTTON)
			{
				if (current.addLastPoint(point(x, y)))
				{
					originalPolygons.push_back(current);
					++colorNo;
					current = polygon(colors[colorNo]);
					glutPostRedisplay();
				}
			}
			if (originalPolygons.size() == 10)
			{
				clipping = true;
				clippedPolygons = vector<polygon>(originalPolygons.size());
			}
		}
	}
	else
	{
		if (state == GLUT_DOWN)
		{
			if (button == GLUT_LEFT_BUTTON)
			{
				clipCorner1 = point(x, y);
				clipCorner2 = point(x, y);
			}
		}
		else
		{
			clippedYet = true;
			for (int i = 0; i < originalPolygons.size(); ++i)
			{
				clippedPolygons[i] = clip(originalPolygons[i],
					min(clipCorner1.x, clipCorner2.x),
					max(clipCorner1.x, clipCorner2.x),
					min(clipCorner1.y, clipCorner2.y),
					max(clipCorner1.y, clipCorner2.y));
			}
			glutPostRedisplay();
		}
	}
}

void onMouseDragged(int x, int y)
{
	clipCorner2 = point(x, y);
	glutPostRedisplay();
}

void draw(void)
{
	clearFramebuffer();

	vector<polygon>& polygons = clippedYet ? clippedPolygons : originalPolygons;

	for (int i = 0; i < polygons.size(); ++i)
	{
		polygons[i].scanConvert();
	}

	if (clipping)
	{
		for (int x = min(clipCorner1.x, clipCorner2.x); x < max(clipCorner1.x, clipCorner2.x); ++x)
		{
			setFramebuffer(x, clipCorner1.y, white);
			setFramebuffer(x, clipCorner2.y, white);
		}
		for (int y = min(clipCorner1.y, clipCorner2.y); y < max(clipCorner1.y, clipCorner2.y); ++y)
		{
			setFramebuffer(clipCorner1.x, y, white);
			setFramebuffer(clipCorner2.x, y, white);
		}
	}
	

	drawit();
}

void init(int* argc, char* argv[])
{
	clearFramebuffer();
	glutInit(argc, argv);
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
	glutInitWindowSize(ImageW, ImageH);
	glutInitWindowPosition(100, 100);
}

int main(int argc, char** argv)
{
	init(&argc, argv);
	glutCreateWindow("Spencer Rawls - Homework 2");
	glutMouseFunc(onClick);
	glutKeyboardFunc(onKeyPressed);
	glutMotionFunc(onMouseDragged);
	glutDisplayFunc(draw);
	glutMainLoop();
	return 0;
}