#ifndef FRAMELIMIT_H
#define FRAMELIMIT_H

enum FrameLimitFlags
{
    FL_NONE = 0,
    FL_FORCE_RENDER = 1 << 0,
    FL_NO_BLOCK = 1 << 1,
};

void Frame_Limiter(FrameLimitFlags flags = FL_FORCE_RENDER);

#endif /* FRAMELIMIT_H */
