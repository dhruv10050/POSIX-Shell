#pragma once
void sigchldHandler(int);
void sigintHandler(int);
void sigtstpHandler(int);
void reapChildren();
