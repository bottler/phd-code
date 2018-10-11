#ifndef JR_REPRESENTATIONS_H
#define JR_REPRESENTATIONS_H

Stroke concat(const Character& ch);
Stroke3d concatWithPenDimension(const Character& ch);

int getRepnLength();
int getRepnNumber();
string getRepnName();
vector<float> representChar(const Character& ch);

void setChoice(int c);


#endif

