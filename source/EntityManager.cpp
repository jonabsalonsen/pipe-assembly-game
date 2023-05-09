#include "EntityManager.h"

EntityManager::EntityManager() {
    numEntities = 0;
    maxNumEntities = NUMBER_OF_TILES;
    Initialize();
}

void EntityManager::Initialize() 
{
    std::fill(tileToEntityMapping, tileToEntityMapping+NUMBER_OF_TILES, -1);

    for (int i = 0; i < NUMBER_OF_TILES; i++) {
        ids[i] = i;
    }

    AddEntity(EntityType::BENT_PIPE, pos{.x=1,.y=1}, true, UP);

    int amount = 16;
    for (int i = 1; i < amount; i++)
    {
        pos A = pos{.x=(i*5) % TILES_COLUMNS,
                    .y=(i*7) % TILES_ROWS};
        while (doesEntityExistAtPosition(A)) {
            A.x = (A.x + 1) % TILES_COLUMNS;
            A.y = (A.y + 1) % TILES_ROWS;
        }
        pos B = pos{.x=(i*11) % TILES_COLUMNS,
                    .y=(i*13) % TILES_ROWS};
        while (doesEntityExistAtPosition(B)) {
            B.x = (B.x + 1) % TILES_COLUMNS;
            B.y = (B.y + 1) % TILES_ROWS;
        }

        AddEntity(EntityType::BENT_PIPE, 
                  A,
                  false, static_cast<Direction>(i % 4));
        AddEntity(EntityType::STRAIGHT_PIPE, 
                  B,
                  false,static_cast<Direction>(i % 4));
    }
}

void EntityManager::AddEntity(EntityType type, pos position, bool isCurrentMovable, Direction orientation) 
{
    if (numEntities >= maxNumEntities)
        return;

    int i = numEntities;
    
    int id = getSmallestAvailableID();
    if (-1 == id)
        return;

    if (!checkBounds(position)) 
    {
        return;
    }
        
    ids[i] = id;
    types[i] = type;
    positions[i] = position;
    int tileIndex = getTileIndexFromPosition(position);
    tileToEntityMapping[tileIndex] = i;
    orientations[i] = orientation;
    isMovable[i] = isCurrentMovable;
    hasMoved[i] = false;

    numEntities++;
}

void EntityManager::DeleteEntity(int id) 
{
    if (numEntities <= 0)
        return;

    // find index of element to be deleted
    int i = getEntityIndexFromId(id);
    if (-1 == i)
        return;

    // move last entity to index of deleted
    ids[i] = ids[numEntities];
    types[i] = types[numEntities];
    positions[i] = positions[numEntities];
    orientations[i] = orientations[numEntities];

    tileToEntityMapping[numEntities] = -1;

    // decrement number of entities
    numEntities--;
}

pos EntityManager::GetAdjacentPosition(pos position, Direction direction) 
{
    switch (direction) {
        case UP:
            return pos{.x=position.x, .y=position.y-1};
        case RIGHT:
            return pos{.x=position.x+1, .y=position.y};
        case DOWN:
            return pos{.x=position.x, .y=position.y+1};
        case LEFT:
            return pos{.x=position.x-1, .y=position.y};
        default:
            return position;
    }
}

void EntityManager::InitializeTurn() {
    std::fill(hasMoved, hasMoved+NUMBER_OF_TILES, false);
    std::fill(deltaPositions, deltaPositions+NUMBER_OF_TILES, posf{0,0});
    std::copy(positions, positions+NUMBER_OF_TILES, positionsBackup);
    std::copy(orientations, orientations+NUMBER_OF_TILES, orientationsBackup);
    std::copy(tileToEntityMapping, tileToEntityMapping+NUMBER_OF_TILES, tileToEntityMappingBackup);
    isTurnOk = true;
}

void EntityManager::InitializeRotation() { 
    std::fill(isTemporarilyMovable, isTemporarilyMovable+NUMBER_OF_TILES, false);
    std::fill(gotPushed, gotPushed+NUMBER_OF_TILES, false);
}

void EntityManager::AbortTurn() {
    std::fill(hasMoved, hasMoved+NUMBER_OF_TILES, false);
    std::copy(positionsBackup, positionsBackup+NUMBER_OF_TILES, positions);
    std::copy(orientationsBackup, orientationsBackup+NUMBER_OF_TILES, orientations);
    std::copy(tileToEntityMappingBackup, tileToEntityMappingBackup+NUMBER_OF_TILES, tileToEntityMapping);
}

void EntityManager::FinalizeTurn() {
    if (isTurnOk) {
        UpdateAllConnections();
        UpdateRotations(rotationCounts);
    }

    else {
        AbortTurn();
    }

    pendingRotation = RotationCounts{0,0};
}

void EntityManager::MoveAllToAdjacent(Direction direction) {

    InitializeTurn();

    for (int i = 0; i < numEntities; i++)
    {
        if (!isMovable[i])
            continue;

        MoveToAdjacentTile(i, direction);
    }

    FinalizeTurn();
}

void EntityManager::PushInDirection(Push push)
{    
    // Move "phantom object" to adjacent position

    pos fromPosition = pos{static_cast<int>(push.fromPosition.x), 
                           static_cast<int>(push.fromPosition.y)};

    pos adjacentPosition = GetAdjacentPosition(fromPosition, push.direction);

    if (!checkBounds(adjacentPosition)) {
        isTurnOk = false;
        return;
    }

    int adjacentIndex = getEntityIndexFromPosition(adjacentPosition);

    if(doesEntityExist(adjacentIndex) && !(hasMoved[adjacentIndex]) && !isMovable[adjacentIndex]) {
        int id = ids[adjacentIndex];
        MoveToAdjacentTile(id, push.direction, true);
    }

    if(doesEntityExistAtPosition(adjacentPosition) && !(isMovable[adjacentIndex] || isTemporarilyMovable[adjacentIndex])) {
        isTurnOk = false;
        return;
    }
}

void EntityManager::MoveToAdjacentTile(int index, Direction direction, bool isRotation)
{
    if (!doesEntityExist(index)) {
        return;
    }

    if (hasMoved[index])
        return;

    pos position = positions[index];

    pos adjacentPosition = GetAdjacentPosition(position, direction);

    if (!checkBounds(adjacentPosition)) {
        isTurnOk = false;
        return;
    }

    int adjacentIndex = getEntityIndexFromPosition(adjacentPosition);

    if(doesEntityExist(adjacentIndex) && !(hasMoved[adjacentIndex])) {

        if (isRotation) {
            if (isMovable[adjacentIndex]) {
                isTemporarilyMovable[index] = true;
                gotPushed[index] = true;
                return;
            }
        }

        int id = ids[adjacentIndex];
        MoveToAdjacentTile(id, direction, isRotation);
    }

    if(doesEntityExistAtPosition(adjacentPosition)) {
        return;
    }

    positions[index] = adjacentPosition;
    int oldTileIndex = getTileIndexFromPosition(position);
    int newTileIndex = getTileIndexFromPosition(adjacentPosition);

    tileToEntityMapping[oldTileIndex] = -1;
    tileToEntityMapping[newTileIndex] = index;

    hasMoved[index] = true;
    if (isRotation) {
        gotPushed[index] = true;
    }
    deltaPositions[index] = posf{static_cast<float>(adjacentPosition.x - position.x), 
                                 static_cast<float>(adjacentPosition.y - position.y)};

}

bool EntityManager::doesEntityExistAtPosition(pos position) 
{
    if (!checkBounds(position))
        return false;

    int index = getEntityIndexFromPosition(position);

    return index >= 0 && index < numEntities;
}


bool EntityManager::doesEntityExist(int index) 
{
    return index >= 0 && index < numEntities;
}

void DisplayHelperGrid( std::vector<Push> pushes) {
    for (int y = 0; y < TILES_ROWS; y++) {
        char row[TILES_COLUMNS*2+1]; 
        for (int x = 0; x < TILES_COLUMNS; x++) {
            bool isPositionFound = false;

            char dir = '#';

            for(auto push : pushes)
            {
                posf p = push.fromPosition;
                isPositionFound |= (static_cast<int>(p.x) == x && static_cast<int>(p.y == y));
                if (isPositionFound) {
                    switch (push.direction) {
                        case (UP):
                            dir = 'A';
                            break;
                        case (DOWN):
                            dir = 'v';
                            break;
                        case (LEFT):
                            dir = '<';
                            break;
                        case (RIGHT):
                            dir = '>';
                            break;
                    }
                }
            }
            row[2*x] = '|';

            row[2*x+1] = isPositionFound ? dir : '_';
        }
    }
}

void EntityManager::RotateAll(Direction direction) {
    pos pivotPosition = positions[0];

    std::vector<Push> pushes = CalculateAllRotationPushes(pivotPosition, direction);

    //DisplayHelperGrid(pushes);

    InitializeRotation();

    pos TempPositions[NUMBER_OF_TILES];
    std::copy(positions, positions+NUMBER_OF_TILES, TempPositions);

    bool canRotationComplete = true;
    float maximumAngle = 7.f;

    std::vector<Push> okPushes;

    for(auto push : pushes)
    {
        InitializeTurn();
        PushInDirection(push);
        FinalizeTurn();

        if (!isTurnOk) {
            maximumAngle = push.priority < maximumAngle ? push.priority : maximumAngle;
            partialRotationSign = 1;
            partialRotationAngle = -push.priority * (direction - 2);
            return;
        }
    }

    InitializeTurn();

    std::copy(gotPushed, gotPushed+NUMBER_OF_TILES, hasMoved);
    for (int i = 0; i < NUMBER_OF_TILES; i++) {
        deltaPositions[i] = gotPushed[i] ? posf{static_cast<float>(positions[i].x - TempPositions[i].x), 
                                                static_cast<float>(positions[i].y - TempPositions[i].y)} : posf{0,0};
    }
    
    for (int i = 0; i < numEntities; i++)
    {
        if (!isMovable[i] && !isTemporarilyMovable[i])
            continue;
        
        RotateMovable(i, direction, pivotPosition);
    }

    switch (direction) {
        case RIGHT:
            pendingRotation.rightRotations++;
            break;
        case LEFT:
            pendingRotation.leftRotations++;
            break;
        default:
            break;
    }

    FinalizeTurn();
}

void EntityManager::RotateMovable(int index, Direction direction, pos pivotPosition)
{
    pos currentPosition = positions[index];
    pos projectedPosition = GetProjectedPosition(currentPosition, pivotPosition, direction);

    if (doesEntityExistAtPosition(projectedPosition)) {
        int toBeRotatedTileIndex = getTileIndexFromEntityIndex(index);
        int projectedPositionTileIndex = getTileIndexFromPosition(projectedPosition);
        bool willCurrentEntityMove = toBeRotatedTileIndex != projectedPositionTileIndex;
        int projectedIndex = getEntityIndexFromPosition(projectedPosition);
        bool isCurrentMovable = (isMovable[projectedIndex] || isTemporarilyMovable[projectedIndex]);
        if ((willCurrentEntityMove && !isCurrentMovable)) {
            isTurnOk = false;
            return;
        }
    }

    if (!checkBounds(projectedPosition)) {
        isTurnOk = false;
        return;    
    }

    int oldTileIndex = getTileIndexFromPosition(positions[index]);
    int newTileIndex = getTileIndexFromPosition(projectedPosition);
    positions[index] = projectedPosition;
    tileToEntityMapping[oldTileIndex] = -1;
    tileToEntityMapping[newTileIndex] = index;
    
    int id = ids[index];
    Rotate(id, direction);
}

pos EntityManager::GetProjectedPosition(pos currentPosition, pos pivotPosition, Direction direction) {
    int deltaX = currentPosition.x-pivotPosition.x;
    int deltaY = currentPosition.y-pivotPosition.y;
    int sign = direction - 2;
    int deltaRotX = -sign * deltaY;
    int deltaRotY = sign * deltaX;
    int x = pivotPosition.x + deltaRotX;
    int y = pivotPosition.y + deltaRotY;
    return pos{.x=x, .y=y};
}

void EntityManager::Rotate(int id, Direction direction) 
{
    int index = getEntityIndexFromId(id);

    Direction d = static_cast<Direction>((orientations[index] + direction) % 4);

    orientations[index] = d;
}

std::vector<Push> EntityManager::CalculateAllRotationPushes(pos pivotPosition, Direction direction)
{
    std::vector<Push> pushes;

    for (int i = 0; i < numEntities; i++)
    {
        if (!isMovable[i] || (positions[i].x == pivotPosition.x && positions[i].y == pivotPosition.y))
            continue;
        
        std::vector<Push> p = GetQuantizedRotationTrajectory(positions[i], pivotPosition, direction);

        //DisplayHelperGrid(p);

        pushes.insert(pushes.end(), p.begin(), p.end());
    }

    std::sort(pushes.begin(), pushes.end(), [](Push a, Push b) { return a.priority < b.priority; });

    return pushes;
}



std::vector<Push> EntityManager::GetQuantizedRotationTrajectory(pos currentPosition, pos pivotPosition, Direction rotationDirection) 
{
    posf currentPoint = posf{static_cast<float>(currentPosition.x-pivotPosition.x), 
                             static_cast<float>(currentPosition.y-pivotPosition.y)};

    int sign = rotationDirection - 2;
    posf endPoint = posf{-currentPoint.y*sign, currentPoint.x*sign};
    float radius = sqrt(currentPoint.x*currentPoint.x + currentPoint.y*currentPoint.y);
    float currentAngle = 0.;
    
    std::vector<posf> outPositions;
    std::vector<float> angles;

    outPositions.push_back(posf{currentPoint.x+pivotPosition.x, currentPoint.y+pivotPosition.y});
    angles.push_back(currentAngle);
    
    for (int i = 0; i < 100; i++) {
        posf outPosition = getNextRotationStep(currentPoint, currentAngle, sign, radius);
        
        outPositions.push_back(posf{outPosition.x+pivotPosition.x, outPosition.y+pivotPosition.y});
        angles.push_back(currentAngle);

        if (outPosition.x == endPoint.x && outPosition.y == endPoint.y)
            break;
    }

    std::vector<Push> pushes;

    for (int i = 0; i < outPositions.size()-1; i++) {
         
        float diffX = outPositions[i+1].x - outPositions[i].x;
        float diffY = outPositions[i+1].y - outPositions[i].y;
        if (abs(diffX) > 1 || abs(diffY) > 1)
            continue;
        Direction d = static_cast<Direction>(diffY*(diffY+1.) + abs(diffX)*(diffX+2.));
        char dir = '_';
        switch (d) {
            case (UP):
                dir = 'A';
                break;
            case (DOWN):
                dir = 'v';
                break;
            case (LEFT):
                dir = '<';
                break;
            case (RIGHT):
                dir = '>';
                break;
        }
        Push push = Push{.priority=angles[i],.fromPosition = outPositions[i],.direction = d};
        pushes.push_back(push);
    }
    
    return pushes;
}

posf EntityManager::getNextRotationStep(posf& pos, float& angle, int sign, float radius) 
{
    if (pos.x > 0.5 && pos.y > 0.5) {
        posf outPosition = idealizedStep(pos, angle, sign, radius);
        return outPosition;
    } else if (pos.x < -0.5 && pos.y > 0.5) {
        posf inPosition = posf{pos.y, -pos.x};
        posf outPosition = idealizedStep(inPosition, angle, sign, radius);
        pos = posf{-inPosition.y, inPosition.x};
        posf transformedOut = posf{-outPosition.y, outPosition.x};
        return transformedOut;
    } else if (pos.x < -0.5 && pos.y < -0.5) {
        posf inPosition = posf{-pos.x, -pos.y};
        posf outPosition = idealizedStep(inPosition, angle, sign, radius);
        pos = posf{-inPosition.x, -inPosition.y};
        posf transformedOut = posf{-outPosition.x, -outPosition.y};
        return transformedOut;
    } else if (pos.x > 0.5 && pos.y < -0.5) {
        posf inPosition = posf{-pos.y, pos.x};
        posf outPosition = idealizedStep(inPosition, angle, sign, radius);
        pos = posf{inPosition.y, -inPosition.x};
        posf transformedOut = posf{outPosition.y, -outPosition.x};
        return transformedOut;
    } else if (abs(pos.x) <= 0.5) {
        float ySign = (pos.y >= 0) - (pos.y < 0);
    
        // SMALL BUG HERE

        float angleDiff = 3.141592 - abs(atan(pos.y/pos.x))*2.;
        angle += angleDiff;

        float nextX = -ySign * sign * 0.5001;
        pos = posf{nextX, pos.y};

        posf outPosition = posf{-ySign * sign, round(pos.y)};
        return outPosition;
    } else if (abs(pos.y) <= 0.5) {
        float xSign = (pos.x >= 0) - (pos.x < 0);

        // SMALL BUG HERE 

        float angleDiff = abs(atan(pos.y/pos.x))*2.;
        angle += angleDiff;

        float nextY = xSign * sign * 0.5001;
        pos = posf{pos.x, nextY};

        posf outPosition = posf{round(pos.x), xSign * sign};
        return outPosition;
    }
    return posf{0.,0.};
}

posf EntityManager::idealizedStep(posf& pos, float& angle, int sign, float radius) 
{
    float currentAngle = atan(pos.y/pos.x);
    posf next = idealizedCoords(pos, sign, radius);
    float angleFromX = acos(next.x/radius);
    float angleFromY = asin(next.y/radius);
    float angleDiffX = abs(angleFromX - currentAngle);
    float angleDiffY = abs(angleFromY - currentAngle);
    if (angleDiffX < angleDiffY) {
        float yFromX = sin(angleFromX)*radius;
        pos.x = next.x;
        pos.y = yFromX;
        angle += angleDiffX;
        posf out = posf{round(pos.x-sign*0.0001f), 
                        round(pos.y+sign*0.0001f)};
        return out;
    } else {
        float xFromY = cos(angleFromY)*radius;
        pos.y = next.y;
        pos.x = xFromY;
        angle += angleDiffY;
        posf out = posf{round(pos.x-sign*0.0001f), 
                        round(pos.y+sign*0.0001f)};
        return out;
    }
}

posf EntityManager::idealizedCoords(posf& currentPosition, int sign, float radius) 
{
    posf nc = currentPosition;
    if (sign == -1) {
        nc.x = round(nc.x + 0.0001) + 0.5;
        nc.y = round(nc.y - 0.0001) - 0.5;
        nc.x = nc.x < radius ? nc.x : radius;
    } else if (sign == 1) {
        nc.x = round(nc.x - 0.0001) - 0.5;
        nc.y = round(nc.y + 0.0001) + 0.5;
        nc.y = nc.y < radius ? nc.y : radius;
    }
    return nc;
}

std::vector<Direction> EntityManager::GetConnectableDirectionsFromId(int id) 
{

    int index = getEntityIndexFromId(id);
    EntityType type = types[index];
    Direction orientation = orientations[index];

    std::vector<Direction> directions;

    switch (type)
    {
        case EntityType::BENT_PIPE:
            directions.push_back(static_cast<Direction>(orientation));
            directions.push_back(static_cast<Direction>((1 + orientation) % 4));
            break;
        case EntityType::STRAIGHT_PIPE:
            directions.push_back(static_cast<Direction>(orientation));
            directions.push_back(static_cast<Direction>((2 + orientation) % 4));
            break;
        default:
            break;
    }

    return directions;
}

void EntityManager::UpdateCurrentConnections(int currentId) 
{
    int index = getEntityIndexFromId(currentId);
    pos currentPosition = positions[index];
    std::vector<Direction> currentConnectableDirections = GetConnectableDirectionsFromId(currentId);

    for(Direction d : currentConnectableDirections)
    {
        pos adjPosition = GetAdjacentPosition(currentPosition, d);
        if (!doesEntityExistAtPosition(adjPosition))
            continue;

        int adjIndex = getEntityIndexFromPosition(adjPosition);
        int adjId = ids[adjIndex];
        if (IsAdjacentConnectable(d, adjId))
            isMovable[adjIndex] = true;
    }
}

bool EntityManager::IsAdjacentConnectable(Direction connectionDirection, int adjId) 
{
    std::vector<Direction> adjConnectableDirections = GetConnectableDirectionsFromId(adjId);
    for(Direction adjDir : adjConnectableDirections)
    {
        if (adjDir == ((connectionDirection + 2) % 4))
            return true;
    }
    return false;
}

void EntityManager::UpdateAllConnections() 
{
    for (int i = 0; i < numEntities; i++) {
        if (!isMovable[i])
            continue;

        int currentId = ids[i];
        UpdateCurrentConnections(currentId);
    }
}

void EntityManager::UpdateRotations(RotationCounts& rotation) {
    rotation.rightRotations+=pendingRotation.rightRotations;
    rotation.leftRotations+=pendingRotation.leftRotations;
    
}

int EntityManager::getSmallestAvailableID() {
    bool isIdAvailable[NUMBER_OF_TILES];
    std::fill(isIdAvailable, isIdAvailable+NUMBER_OF_TILES, true);

    for (int i = 0; i < numEntities; i++)
    {
        isIdAvailable[ids[i]] = false;
    }

    for (int i = 0; i < NUMBER_OF_TILES; i++) {
        if (isIdAvailable[i])
            return i;
    }

    return -1;
}

int EntityManager::getEntityIndexFromId(int id) 
{
    for (int i = 0; i < numEntities; i++)
    {
        if (ids[i] == id)
            return i;
    }
    return -1;
}

int EntityManager::getEntityIndexFromPosition(pos position) 
{
    int tileIndex = getTileIndexFromPosition(position);
    return tileToEntityMapping[tileIndex];
}

int EntityManager::getTileIndexFromPosition(pos position) 
{
    if (!checkBounds(position)) {
        return -1;
    }
         
    int index = position.x + position.y * TILES_COLUMNS;
    return index;
}

int EntityManager::getTileIndexFromEntityIndex(int i) 
{
    pos position = positions[i];
    return getTileIndexFromPosition(position);
}

bool EntityManager::checkBounds(pos position) 
{
    return (position.x >= 0) &&
        (position.y >= 0) &&
        (position.x <= TILES_COLUMNS - 1) &&
        (position.y <= TILES_ROWS - 1);
}