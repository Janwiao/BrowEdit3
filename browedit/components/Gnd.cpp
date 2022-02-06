#include "Gnd.h"
#include <browedit/util/Util.h>
#include <browedit/util/FileIO.h>
#include <iostream>

Gnd::Gnd(const std::string& fileName)
{
	auto file = util::FileIO::open("data\\" + fileName);
	if (!file)
	{
		width = 0;
		height = 0;
		std::cerr << "GND: Unable to open gnd file: " << fileName << ".gnd" << std::endl;
		return;
	}
	std::cout<< "GND: Reading gnd file" << std::endl;
	char header[4];
	file->read(header, 4);
	if (header[0] == 'G' && header[1] == 'R' && header[2] == 'G' && header[3] == 'N')
	{
		file->read(reinterpret_cast<char*>(&version), sizeof(short));
		version = util::swapShort(version);
	}
	else
	{
		version = 0;
		std::cerr<< "GND: Invalid GND file" << std::endl;
		return;
	}

	int textureCount = 0;

	if (version > 0)
	{
		file->read(reinterpret_cast<char*>(&width), sizeof(int));
		file->read(reinterpret_cast<char*>(&height), sizeof(int));
		file->read(reinterpret_cast<char*>(&tileScale), sizeof(float));
		file->read(reinterpret_cast<char*>(&textureCount), sizeof(int));
		file->read(reinterpret_cast<char*>(&maxTexName), sizeof(int)); //80
	}
	else
	{
		file->seekg(6, std::ios_base::cur);//TODO: test this
		file->read(reinterpret_cast<char*>(&width), sizeof(int));
		file->read(reinterpret_cast<char*>(&height), sizeof(int));
		file->read(reinterpret_cast<char*>(&textureCount), sizeof(int));
	}

	textures.reserve(textureCount);
	for (int i = 0; i < textureCount; i++)
	{
		Texture* texture = new Texture();
		texture->file = util::FileIO::readString(file, 40);
		texture->name = util::FileIO::readString(file, 40);
		//texture->texture = new Texture("data/texture/" + texture->file); //TODO: cached loader
		textures.push_back(texture);
	}


	if (version > 0)
	{
		int lightmapCount;
		file->read(reinterpret_cast<char*>(&lightmapCount), sizeof(int));
		file->read(reinterpret_cast<char*>(&lightmapWidth), sizeof(int));
		file->read(reinterpret_cast<char*>(&lightmapHeight), sizeof(int));
		file->read(reinterpret_cast<char*>(&gridSizeCell), sizeof(int));

		//Fix lightmap format if it was invalid. by Henko
		if (lightmapWidth != 8 || lightmapHeight != 8 || gridSizeCell != 1)
		{
			std::cerr << "GND: Invalid Lightmap Format in " << fileName << ".gnd" << std::endl;
			lightmapWidth = 8;
			lightmapHeight = 8;
			gridSizeCell = 1;
		}

		lightmaps.reserve(lightmapCount);
		for (int i = 0; i < lightmapCount; i++)
		{
			Lightmap* lightmap = new Lightmap();
			file->read((char*)lightmap->data, 256);
			lightmaps.push_back(lightmap);
		}

		int tileCount;
		file->read(reinterpret_cast<char*>(&tileCount), sizeof(int));
		std::cout << "GND: Tilecount: " << tileCount << std::endl;
		tiles.reserve(tileCount);
		for (int i = 0; i < tileCount; i++)
		{
			Tile* tile = new Tile();

			file->read(reinterpret_cast<char*>(&tile->v1.x), sizeof(float));
			file->read(reinterpret_cast<char*>(&tile->v2.x), sizeof(float));
			file->read(reinterpret_cast<char*>(&tile->v3.x), sizeof(float));
			file->read(reinterpret_cast<char*>(&tile->v4.x), sizeof(float));
			file->read(reinterpret_cast<char*>(&tile->v1.y), sizeof(float));
			file->read(reinterpret_cast<char*>(&tile->v2.y), sizeof(float));
			file->read(reinterpret_cast<char*>(&tile->v3.y), sizeof(float));
			file->read(reinterpret_cast<char*>(&tile->v4.y), sizeof(float));

			file->read(reinterpret_cast<char*>(&tile->textureIndex), sizeof(short));
			file->read(reinterpret_cast<char*>(&tile->lightmapIndex), sizeof(unsigned short));


			if (tile->lightmapIndex < 0 || tile->lightmapIndex == (unsigned short)-1)
			{
				std::cout << "GND: Lightmapindex < 0" << std::endl;
				tile->lightmapIndex = 0;
			}

			tile->color.b = (unsigned char)file->get();
			tile->color.g = (unsigned char)file->get();
			tile->color.r = (unsigned char)file->get();
			tile->color.a = (unsigned char)file->get();
			tiles.push_back(tile);
		}

		cubes.resize(width, std::vector<Cube*>(height, NULL));
		for (int y = 0; y < height; y++)
		{
			for (int x = 0; x < width; x++)
			{
				Cube* cube = new Cube();
				file->read(reinterpret_cast<char*>(&cube->h1), sizeof(float));
				file->read(reinterpret_cast<char*>(&cube->h2), sizeof(float));
				file->read(reinterpret_cast<char*>(&cube->h3), sizeof(float));
				file->read(reinterpret_cast<char*>(&cube->h4), sizeof(float));
				cube->calcNormal();

				if (version >= 0x0106)
				{
					file->read(reinterpret_cast<char*>(&cube->tileUp), sizeof(int));
					file->read(reinterpret_cast<char*>(&cube->tileSide), sizeof(int));
					file->read(reinterpret_cast<char*>(&cube->tileFront), sizeof(int));
				}
				else
				{
					unsigned short up, side, front;
					file->read(reinterpret_cast<char*>(&up), sizeof(unsigned short));
					file->read(reinterpret_cast<char*>(&side), sizeof(unsigned short));
					file->read(reinterpret_cast<char*>(&front), sizeof(unsigned short));

					cube->tileUp = up;
					cube->tileSide = side;
					cube->tileFront = front;
				}

				if (cube->tileUp >= (int)tiles.size())
				{
					std::cout << "GND: Wrong value for tileup at " << x << ", " << y << "Found " << cube->tileUp << ", but only " << tiles.size() << " tiles found" << std::endl;
					cube->tileUp = -1;
				}
				if (cube->tileSide >= (int)tiles.size())
				{
					std::cout << "GND: Wrong value for tileside at " << x << ", " << y << std::endl;
					cube->tileSide = -1;
				}
				if (cube->tileFront >= (int)tiles.size())
				{
					std::cout << "GND: Wrong value for tilefront at " << x << ", " << y << std::endl;
					cube->tileFront = -1;
				}

				cubes[x][y] = cube;
			}
		}
	}
	else
	{
		//TODO: port code...too lazy for now
	}
	delete file;
	std::cout << "GND: Done reading gnd file" << std::endl;

	for (int x = 1; x < width - 1; x++)
		for (int y = 1; y < height - 1; y++)
			cubes[x][y]->calcNormals(this, x, y);
	std::cout << "GND: Done calculating normals" << std::endl;
}



glm::vec3 Gnd::rayCast(const math::Ray& ray)
{
	float f = 0;
	for (auto x = 0; x < cubes.size(); x++)
	{
		for (auto y = 0; y < cubes[x].size(); y++)
		{
			Gnd::Cube* cube = cubes[x][y];

			if (cube->tileUp != -1)
			{
				Gnd::Tile* tile = tiles[cube->tileUp];

				glm::vec3 v1(10 * x, -cube->h3, 10 * height - 10 * y);
				glm::vec3 v2(10 * x + 10, -cube->h4, 10 * height - 10 * y);
				glm::vec3 v3(10 * x, -cube->h1, 10 * height - 10 * y + 10);
				glm::vec3 v4(10 * x + 10, -cube->h2, 10 * height - 10 * y + 10);

				{
					std::vector<glm::vec3> v{ v3, v2, v1 };
					if (ray.LineIntersectPolygon(v, f))
						return ray.origin + f * ray.dir;
				}
				{
					std::vector<glm::vec3> v{ v4, v2, v3 };
					if (ray.LineIntersectPolygon(v, f))
						return ray.origin + f * ray.dir;
				}
			}
			if (cube->tileFront != -1 && x < width - 1)
			{
				Gnd::Tile* tile = tiles[cube->tileFront];
				assert(tile->lightmapIndex >= 0);

				glm::vec3 v1(10 * x + 10, -cube->h2, 10 * height - 10 * y + 10);
				glm::vec3 v2(10 * x + 10, -cube->h4, 10 * height - 10 * y);
				glm::vec3 v3(10 * x + 10, -cubes[x + 1][y]->h1, 10 * height - 10 * y + 10);
				glm::vec3 v4(10 * x + 10, -cubes[x + 1][y]->h3, 10 * height - 10 * y);

				{
					std::vector<glm::vec3> v{ v3, v2, v1 };
					if (ray.LineIntersectPolygon(v, f))
						return ray.origin + f * ray.dir;
				}
				{
					std::vector<glm::vec3> v{ v4, v2, v3 };
					if (ray.LineIntersectPolygon(v, f))
						return ray.origin + f * ray.dir;
				}
			}
			if (cube->tileSide != -1 && y < height - 1)
			{
				Gnd::Tile* tile = tiles[cube->tileSide];

				glm::vec3 v1(10 * x, -cube->h3, 10 * height - 10 * y);
				glm::vec3 v2(10 * x + 10, -cube->h4, 10 * height - 10 * y);
				glm::vec3 v4(10 * x + 10, -cubes[x][y + 1]->h2, 10 * height - 10 * y);
				glm::vec3 v3(10 * x, -cubes[x][y + 1]->h1, 10 * height - 10 * y);

				{
					std::vector<glm::vec3> v{ v3, v2, v1 };
					if (ray.LineIntersectPolygon(v, f))
						return ray.origin + f * ray.dir;
				}
				{
					std::vector<glm::vec3> v{ v4, v2, v3 };
					if (ray.LineIntersectPolygon(v, f))
						return ray.origin + f * ray.dir;
				}
			}
		}
	}
	return glm::vec3(0, 0, 0);
}


void Gnd::makeLightmapsUnique()
{
	makeTilesUnique();
	std::set<int> taken;
	for (Tile* t : tiles)
	{
		if (taken.find(t->lightmapIndex) == taken.end())
			taken.insert(t->lightmapIndex);
		else
		{
			Lightmap* l = new Lightmap(*lightmaps[t->lightmapIndex]);
			t->lightmapIndex = (unsigned short)lightmaps.size();
			lightmaps.push_back(l);
		}
	}
}

void Gnd::makeLightmapsClear()
{
	lightmaps.clear();
	Lightmap* l = new Lightmap();
	memset(l->data, 255, 64);
	memset(l->data + 64, 0, 256 - 64);
	lightmaps.push_back(l);

	for (Tile* t : tiles)
		t->lightmapIndex = 0;
}
void Gnd::makeLightmapBorders()
{
	makeLightmapsUnique();
	std::cout<< "Fixing borders" << std::endl;
	for (int x = 0; x < width; x++)
	{
		for (int y = 0; y < height; y++)
		{
			Gnd::Cube* cube = cubes[x][y];
			int tileId = cube->tileUp;
			if (tileId != -1)
			{
				Gnd::Tile* tile = tiles[tileId];
				assert(tile && tile->lightmapIndex != -1);
				Gnd::Lightmap* lightmap = lightmaps[tile->lightmapIndex];

				for (int i = 0; i < 8; i++)
				{
					lightmap->data[i + 8 * 0] = getLightmapBrightness(x, y - 1, i, 6);
					lightmap->data[i + 8 * 7] = getLightmapBrightness(x, y + 1, i, 1);
					lightmap->data[0 + 8 * i] = getLightmapBrightness(x - 1, y, 6, i);
					lightmap->data[7 + 8 * i] = getLightmapBrightness(x + 1, y, 1, i);

					for (int c = 0; c < 3; c++)
					{
						lightmap->data[64 + 3 * (i + 8 * 0) + c] = getLightmapColor(x, y - 1, i, 6)[c];
						lightmap->data[64 + 3 * (i + 8 * 7) + c] = getLightmapColor(x, y + 1, i, 1)[c];
						lightmap->data[64 + 3 * (0 + 8 * i) + c] = getLightmapColor(x - 1, y, 6, i)[c];
						lightmap->data[64 + 3 * (7 + 8 * i) + c] = getLightmapColor(x + 1, y, 1, i)[c];

					}

				}
			}
			tileId = cube->tileSide;
			if (tileId != -1)
			{
				Gnd::Tile* tile = tiles[tileId];
				assert(tile && tile->lightmapIndex != -1);
				Gnd::Lightmap* lightmap = lightmaps[tile->lightmapIndex];

				auto otherCube = cubes[x - 1][y];
				if (otherCube->tileSide != -1)
				{
					auto otherTile = tiles[otherCube->tileSide];
					auto otherLightmap = lightmaps[otherTile->lightmapIndex];

					for (int i = 0; i < 8; i++)
						lightmap->data[0 + 8 * i] = otherLightmap->data[6 + 8 * i];
				}
				else
					for (int i = 0; i < 8; i++)
						lightmap->data[0 + 8 * i] = lightmap->data[1 + 8 * i];

				otherCube = cubes[x + 1][y];
				if (otherCube->tileSide != -1)
				{
					auto otherTile = tiles[otherCube->tileSide];
					auto otherLightmap = lightmaps[otherTile->lightmapIndex];

					for (int i = 0; i < 8; i++)
						lightmap->data[7 + 8 * i] = otherLightmap->data[1 + 8 * i];
				}
				else
					for (int i = 0; i < 8; i++)
						lightmap->data[7 + 8 * i] = lightmap->data[6 + 8 * i];

				//top and bottom
				otherCube = cubes[x][y + 1];
				if (otherCube->tileUp != -1)
				{
					auto otherTile = tiles[otherCube->tileUp];
					auto otherLightmap = lightmaps[otherTile->lightmapIndex];

					for (int i = 0; i < 8; i++)
						lightmap->data[i + 8 * 7] = otherLightmap->data[i + 8 * 1]; //bottom
				}
				else
					for (int i = 0; i < 8; i++)
						lightmap->data[i + 8 * 7] = lightmap->data[i + 8 * 6];


				otherCube = cubes[x][y];
				if (otherCube->tileUp != -1)
				{
					auto otherTile = tiles[otherCube->tileUp];
					auto otherLightmap = lightmaps[otherTile->lightmapIndex];

					for (int i = 0; i < 8; i++)
						lightmap->data[i + 8 * 0] = otherLightmap->data[i + 8 * 1]; //bottom
				}
				else
					for (int i = 0; i < 8; i++)
						lightmap->data[i + 8 * 0] = lightmap->data[i + 8 * 1];
			}

			tileId = cube->tileFront;
			if (tileId != -1)
			{
				Gnd::Tile* tile = tiles[tileId];
				assert(tile && tile->lightmapIndex != -1);
				Gnd::Lightmap* lightmap = lightmaps[tile->lightmapIndex];

				if (y < width)
				{
					auto otherCube = cubes[x][y + 1];
					if (otherCube->tileFront != -1)
					{
						auto otherTile = tiles[otherCube->tileFront];
						auto otherLightmap = lightmaps[otherTile->lightmapIndex];

						for (int i = 0; i < 8; i++)
							lightmap->data[0 + 8 * i] = otherLightmap->data[6 + 8 * i];
					}
					else
						for (int i = 0; i < 8; i++)
							lightmap->data[0 + 8 * i] = lightmap->data[1 + 8 * i];
				}

				if (y > 0)
				{
					auto otherCube = cubes[x][y - 1];
					if (otherCube->tileFront != -1)
					{
						auto otherTile = tiles[otherCube->tileFront];
						auto otherLightmap = lightmaps[otherTile->lightmapIndex];

						for (int i = 0; i < 8; i++)
							lightmap->data[7 + 8 * i] = otherLightmap->data[1 + 8 * i];
					}
					else
						for (int i = 0; i < 8; i++)
							lightmap->data[7 + 8 * i] = lightmap->data[6 + 8 * i];
				}

				if (x < width)
				{
					//top and bottom
					auto otherCube = cubes[x + 1][y];
					if (otherCube->tileUp != -1)
					{
						auto otherTile = tiles[otherCube->tileUp];
						auto otherLightmap = lightmaps[otherTile->lightmapIndex];

						for (int i = 0; i < 8; i++)
							lightmap->data[i + 8 * 7] = otherLightmap->data[i + 8 * 1]; //bottom
					}
					else
						for (int i = 0; i < 8; i++)
							lightmap->data[i + 8 * 7] = lightmap->data[i + 8 * 6];
				}

				auto otherCube = cubes[x][y];
				if (otherCube->tileUp != -1)
				{
					auto otherTile = tiles[otherCube->tileUp];
					auto otherLightmap = lightmaps[otherTile->lightmapIndex];

					for (int i = 0; i < 8; i++)
						lightmap->data[i + 8 * 0] = otherLightmap->data[i + 8 * 1]; //bottom
				}
				else
					for (int i = 0; i < 8; i++)
						lightmap->data[i + 8 * 0] = lightmap->data[i + 8 * 1];
			}

		}
	}


}


void Gnd::makeLightmapBorders(int x, int y)
{
	if (x < 0 || x >= width || y < 0 || y >= height)
		return;
	Gnd::Cube* cube = cubes[x][y];
	int tileId = cube->tileUp;
	if (tileId == -1)
		return;
	Gnd::Tile* tile = tiles[tileId];
	assert(tile && tile->lightmapIndex != -1);
	Gnd::Lightmap* lightmap = lightmaps[tile->lightmapIndex];

	for (int i = 0; i < 8; i++)
	{
		lightmap->data[i + 8 * 0] = getLightmapBrightness(x, y - 1, i, 6);
		lightmap->data[i + 8 * 7] = getLightmapBrightness(x, y + 1, i, 1);
		lightmap->data[0 + 8 * i] = getLightmapBrightness(x - 1, y, 6, i);
		lightmap->data[7 + 8 * i] = getLightmapBrightness(x + 1, y, 1, i);

		for (int c = 0; c < 3; c++)
		{
			lightmap->data[64 + 3 * (i + 8 * 0) + c] = getLightmapColor(x, y - 1, i, 6)[c];
			lightmap->data[64 + 3 * (i + 8 * 7) + c] = getLightmapColor(x, y + 1, i, 1)[c];
			lightmap->data[64 + 3 * (0 + 8 * i) + c] = getLightmapColor(x - 1, y, 6, i)[c];
			lightmap->data[64 + 3 * (7 + 8 * i) + c] = getLightmapColor(x + 1, y, 1, i)[c];

		}
	}
}


int Gnd::getLightmapBrightness(int x, int y, int lightmapX, int lightmapY)
{
	if (x < 0 || y < 0 || x >= width || y >= height)
		return 0;

	Gnd::Cube* cube = cubes[x][y];
	int tileId = cube->tileUp;
	if (tileId == -1)
		return 0;
	Gnd::Tile* tile = tiles[tileId];
	assert(tile && tile->lightmapIndex != -1);
	Gnd::Lightmap* lightmap = lightmaps[tile->lightmapIndex];

	return lightmap->data[lightmapX + 8 * lightmapY];

}

glm::ivec3 Gnd::getLightmapColor(int x, int y, int lightmapX, int lightmapY)
{
	if (x < 0 || y < 0 || x >= width || y >= height)
		return glm::ivec3(0, 0, 0);

	Gnd::Cube* cube = cubes[x][y];
	int tileId = cube->tileUp;
	if (tileId == -1)
		return glm::ivec3(0, 0, 0);
	Gnd::Tile* tile = tiles[tileId];
	assert(tile && tile->lightmapIndex != -1);
	Gnd::Lightmap* lightmap = lightmaps[tile->lightmapIndex];

	return glm::ivec3(
		lightmap->data[64 + (lightmapX + 8 * lightmapY) * 3 + 0],
		lightmap->data[64 + (lightmapX + 8 * lightmapY) * 3 + 1],
		lightmap->data[64 + (lightmapX + 8 * lightmapY) * 3 + 2]);


}

void Gnd::cleanLightmaps()
{
	std::cout<< "Lightmap cleanup, starting with " << lightmaps.size() << " lightmaps" <<std::endl;
	std::map<unsigned char, std::vector<std::size_t>> lookup;

	for (int i = 0; i < (int)lightmaps.size(); i++)
	{
		unsigned char hash = lightmaps[i]->hash();
		bool found = false;
		if (lookup.find(hash) != lookup.end())
		{
			for (const auto ii : lookup[hash])
			{
				if ((*lightmaps[i]) == (*lightmaps[ii]))
				{// if it is found
					assert(i > ii);
					//change all tiles with lightmap i to ii
					for (auto tile : tiles)
						if (tile->lightmapIndex == i)
							tile->lightmapIndex = (unsigned short)ii;
						else if (tile->lightmapIndex > i)
							tile->lightmapIndex--;
					//remove lightmap i
					delete lightmaps[i];
					lightmaps.erase(lightmaps.begin() + i);
					i--;
					found = true;
					break;
				}
			}
		}
		if (!found)
		{
			lookup[hash].push_back(i);
		}

	}

	std::cout<< "Lightmap cleanup, ending with " << lightmaps.size() << " lightmaps" << std::endl;

}


void Gnd::makeLightmapsSmooth()
{
	std::cout << "Smoothing..." << std::endl;
	for (int x = 0; x < width; x++)
	{
		for (int y = 0; y < height; y++)
		{
			Gnd::Cube* cube = cubes[x][y];
			int tileId = cube->tileUp;
			if (tileId == -1)
				continue;
			Gnd::Tile* tile = tiles[tileId];
			assert(tile && tile->lightmapIndex != -1);
			Gnd::Lightmap* lightmap = lightmaps[tile->lightmapIndex];

			char newData[64];

			for (int xx = 1; xx < 7; xx++)
			{
				for (int yy = 1; yy < 7; yy++)
				{
					int total = 0;
					for (int xxx = xx - 1; xxx <= xx + 1; xxx++)
						for (int yyy = yy - 1; yyy <= yy + 1; yyy++)
							total += lightmap->data[xxx + 8 * yyy];
					newData[xx + 8 * yy] = total / 9;
				}
			}
			memcpy(lightmap->data, newData, 64 * sizeof(char));
		}
	}

}

void Gnd::cleanTiles()
{
	std::cout << "Tiles cleanup, starting with " << tiles.size() << " tiles" << std::endl;
	std::set<int> used;
	for (int y = 0; y < height; y++)
		for (int x = 0; x < width; x++)
		{
			used.insert(cubes[x][y]->tileUp);
			used.insert(cubes[x][y]->tileFront);
			used.insert(cubes[x][y]->tileSide);
		}


	std::list<std::size_t> toRemove;
	for (std::size_t i = 0; i < tiles.size(); i++)
		if (used.find((int)i) == used.end())
			toRemove.push_back(i);
	toRemove.reverse();

	for (std::size_t i : toRemove)
	{
		tiles.erase(tiles.begin() + i);
		for (int y = 0; y < height; y++)
		{
			for (int x = 0; x < width; x++)
			{
				if (cubes[x][y]->tileUp > i)
					cubes[x][y]->tileUp--;
				if (cubes[x][y]->tileSide > i)
					cubes[x][y]->tileSide--;
				if (cubes[x][y]->tileFront > i)
					cubes[x][y]->tileFront--;
			}
		}
	}
	std::cout<< "Tiles cleanup, ending with " << tiles.size() << " tiles" << std::endl;
}


void Gnd::makeTilesUnique()
{
	std::set<int> taken;
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			if (cubes[x][y]->tileUp == -1)
				continue;
			if (taken.find(cubes[x][y]->tileUp) == taken.end())
				taken.insert(cubes[x][y]->tileUp);
			else
			{
				Tile* t = new Tile(*tiles[cubes[x][y]->tileUp]);
				cubes[x][y]->tileUp = (int)tiles.size();
				tiles.push_back(t);
			}
		}
	}
}








Gnd::Texture::Texture()
{
	texture = nullptr;
}

Gnd::Texture::~Texture()
{
	if (texture)
		delete[] texture; //TODO: resource manager
	texture = nullptr;
}

void Gnd::Cube::calcNormal()
{
	/*
		1----2
		|\   |
		| \  |
		|  \ |
		|   \|
		3----4
	*/
	glm::vec3 v1(10, -h1, 0);
	glm::vec3 v2(0, -h2, 0);
	glm::vec3 v3(10, -h3, 10);
	glm::vec3 v4(0, -h4, 10);

	glm::vec3 normal1 = glm::normalize(glm::cross(v4 - v3, v1 - v3));
	glm::vec3 normal2 = glm::normalize(glm::cross(v1 - v2, v4 - v2));
	normal = glm::normalize(normal1 + normal2);
	for (int i = 0; i < 4; i++)
		normals[i] = normal;
}


void Gnd::Cube::calcNormals(Gnd* gnd, int x, int y)
{
	for (int i = 0; i < 4; i++)
	{
		normals[i] = glm::vec3(0, 0, 0);
		for (int ii = 0; ii < 4; ii++)
		{
			int xx = (ii % 2) * ((i % 2 == 0) ? -1 : 1);
			int yy = (ii / 2) * (i < 2 ? -1 : 1);
			if (x + xx >= 0 && x + xx < gnd->width && y + yy >= 0 && y + yy < gnd->height)
				normals[i] += gnd->cubes[x + xx][y + yy]->normal;
		}
		normals[i] = glm::normalize(normals[i]);
	}
}



std::vector<glm::vec3> Gnd::getMapQuads()
{
	std::vector<glm::vec3> quads;

	for (int x = 0; x < width; x++)
	{
		for (int y = 0; y < height; y++)
		{
			auto cube = cubes[x][y];
			if (cube->tileUp != -1)
			{
				/*quads.push_back(glm::vec3((x + 0) * 10, -cube->heights[2], 10 * height - (y + 1) * 10 + 10));
				quads.push_back(glm::vec3((x + 0) * 10, -cube->heights[0], 10 * height - (y + 0) * 10 + 10));
				quads.push_back(glm::vec3((x + 1) * 10, -cube->heights[1], 10 * height - (y + 0) * 10 + 10));
				quads.push_back(glm::vec3((x + 1) * 10, -cube->heights[3], 10 * height - (y + 1) * 10 + 10));*/
			}
			if (cube->tileSide != -1)
			{
				quads.push_back(glm::vec3(10 * x, -cube->h3, 10 * height - 10 * y));
				quads.push_back(glm::vec3(10 * x + 10, -cube->h4, 10 * height - 10 * y));
				quads.push_back(glm::vec3(10 * x + 10, -cubes[x][y + 1]->h2, 10 * height - 10 * y));
				quads.push_back(glm::vec3(10 * x, -cubes[x][y + 1]->h1, 10 * height - 10 * y));
			}
			if (cube->tileFront != -1)
			{
				quads.push_back(glm::vec3(10 * x + 10, -cube->h2, 10 * height - 10 * y + 10));
				quads.push_back(glm::vec3(10 * x + 10, -cube->h4, 10 * height - 10 * y));
				quads.push_back(glm::vec3(10 * x + 10, -cubes[x + 1][y]->h3, 10 * height - 10 * y));
				quads.push_back(glm::vec3(10 * x + 10, -cubes[x + 1][y]->h1, 10 * height - 10 * y + 10));
			}
		}
	}

	/*for (int x = 0; x < width; x++)
	{
		for (int y = 0; y < height; y++)
		{
			if (cubes[x][y]->tileUp != -1)
			{
				file << "f ";
				for (int i = 0; i < 4; i++)
					file << (x + y * width) * 4 + i + 1 << " ";
				file << std::endl;
			}
			if (cubes[x][y]->tileSide != -1)
			{
				file << "f ";
				file << (x + y * width) * 4 + 2 + 1 << " ";
				file << (x + y * width) * 4 + 3 + 1 << " ";
				file << (x + (y + 1) * width) * 4 + 0 + 1 << " ";
				file << (x + (y + 1) * width) * 4 + 1 + 1 << " ";
				file << std::endl;
			}

			if (cubes[x][y]->tileFront != -1)
			{
				file << "f ";
				file << (x + y * width) * 4 + 3 + 1 << " ";
				file << (x + y * width) * 4 + 0 + 1 << " ";
				file << ((x + 1) + y * width) * 4 + 1 + 1 << " ";
				file << ((x + 1) + y * width) * 4 + 2 + 1 << " ";
				file << std::endl;
			}
		}
	}*/
	return quads;
}

const unsigned char Gnd::Lightmap::hash() const //actually a crc, but will work
{
#define POLY 0x82f63b78
	unsigned char crc = ~0;
	for (int i = 0; i < 256; i++) {
		crc ^= data[i];
		for (int k = 0; k < 8; k++)
			crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
	}
	return ~crc;
}


const unsigned char Gnd::Tile::hash() const
{
#define POLY 0x82f63b78
	unsigned char crc = ~0;
	for (int i = 0; i < sizeof(Tile); i++) {
		crc ^= ((char*)this)[i];
		for (int k = 0; k < 8; k++)
			crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
	}
	return ~crc;
}

bool Gnd::Lightmap::operator==(const Lightmap& other) const
{
	return memcmp(data, other.data, 256) == 0;
}


bool Gnd::Tile::operator==(const Gnd::Tile& o) const
{
	return	v1 == o.v1 &&
		v2 == o.v2 &&
		v3 == o.v3 &&
		v4 == o.v4 &&
		textureIndex == o.textureIndex &&
		lightmapIndex == o.lightmapIndex &&
		color == o.color;
}
