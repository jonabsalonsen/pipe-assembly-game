#pragma once

#include "common.h"

struct move {
    int entityIndex;
    int oldTileIndex;
    int newTileIndex;
};

class EntityManager {
    public:
    unsigned int maxNumEntities;
    unsigned int numEntities;

    // identity
    int ids[NUMBER_OF_TILES];
    EntityType types[NUMBER_OF_TILES];
    bool isMovable[NUMBER_OF_TILES];
    bool isTemporarilyMovable[NUMBER_OF_TILES];
    bool gotPushed[NUMBER_OF_TILES];
    bool hasMoved[NUMBER_OF_TILES];

    // transform
    pos positions[NUMBER_OF_TILES];
    pos positionsBackup[NUMBER_OF_TILES];
    Direction orientations[NUMBER_OF_TILES];
    Direction orientationsBackup[NUMBER_OF_TILES];
    int tileToEntityMapping[NUMBER_OF_TILES];
    int tileToEntityMappingBackup[NUMBER_OF_TILES];
    posf deltaPositions[NUMBER_OF_TILES];

    RotationCounts rotationCounts = RotationCounts{0,0};
    RotationCounts pendingRotation = RotationCounts{0,0};
    float partialRotationAngle = 0.0f;
    int partialRotationSign = 0;

    bool isTurnOk;
    bool isRotationOk;

    EntityManager();

    void Initialize();
    void InitializeTurn();
    void InitializeRotation();
    void FinalizeTurn();
    void AbortTurn();
    void AddEntity(EntityType type, pos position, bool isMovable, Direction orientation);
    void DeleteEntity(int id);
    void DeleteEntity(pos position); 

    void MoveEntity(int id, pos position);
    void MoveEntity(pos current, pos destination);
    pos GetAdjacentPosition(pos position, Direction direction);
    void MoveAllToAdjacent(Direction direction);
    void PushInDirection(Push push);
    void MoveToAdjacentTile(int id, Direction direction, bool isRotation = false);
    bool doesEntityExistAtPosition(pos position);
    bool doesEntityExist(int index);

    void RotateAll(Direction direction);
    void PartialRotation(float angleAmount);
    void RotateMovable(int index, Direction direction, pos pivotPosition);
    pos GetProjectedPosition(pos currentPosition, pos pivotPosition, Direction direction);
    void Rotate(int id, Direction direction);
    float ComputeCollisionAngle();
    std::vector<Push> CalculateAllRotationPushes(pos pivotPosition, Direction direction);
    std::vector<Push> GetQuantizedRotationTrajectory(pos currentPosition, pos pivotPosition, Direction rotationDirection);
    posf getNextRotationStep(posf& currentPosition, float& currentAngle, int sign, float radius);
    posf idealizedStep(posf& currentPosition, float& angle, int sign, float radius);
    posf idealizedCoords(posf& currentPosition, int sign, float radius);
    std::vector<Direction> GetConnectableDirectionsFromId(int id);
    void UpdateCurrentConnections(int id);

    bool CanConnectInDirection(int id, Direction direction);
    bool IsAdjacentConnectable(Direction connectionDirection, int adjIndex);
    void UpdateAllConnections();
    void UpdateRotations(RotationCounts& rotation);

    int getSmallestAvailableID();
    int getEntityIndexFromId(int id);
    int getEntityIndexFromPosition(pos position);
    int getTileIndexFromPosition(pos position);
    int getTileIndexFromEntityIndex(int i);
    pos getPositionFromTileIndex(int tileIndex);

    bool checkBounds(pos position);
};