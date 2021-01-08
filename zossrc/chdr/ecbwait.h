#ifndef ECBWAIT_H
#define ECBWAIT_H

#include "ihaecb.h"
#include "z.h"

typedef void(TIMER_EXIT)(void *);
typedef struct
{
  unsigned char value[4];
} TIMER_ID;

#if defined(__IBM_METAL__)
#define STIMERM_MODEL(stimermm)                               \
  __asm(                                                      \
      "*                                                  \n" \
      " STIMERM SET,"                                         \
      "MF=L                                               \n" \
      "*                                                    " \
      : "DS"(stimermm));
#else
#define STIMERM_MODEL(stimermm)
#endif

#if defined(__IBM_METAL__)
#define STIMER_WAIT(time)                                     \
  __asm(                                                      \
      "*                                                  \n" \
      " TAM   ,         AMODE64??                         \n" \
      " JM    *+4+4+2   No, skip switching                \n" \
      " OILH  2,X'8000' Set AMODE31 flag                  \n" \
      " SAM31 ,         Set AMODE31                       \n" \
      "*                                                  \n" \
      " SYSSTATE PUSH       Save SYSSTATE                 \n" \
      " SYSSTATE AMODE64=NO                               \n" \
      "*                                                  \n" \
      " STIMER WAIT,BINTVL=(%0)                           \n" \
      "*                                                  \n" \
      " TMLH  2,X'8000' Did we switch AMODE??             \n" \
      " JNO   *+4+2     No, skip restore                  \n" \
      " SAM64 ,         Set AMODE64                       \n" \
      "*                                                  \n" \
      " NILH  2,X'7FFF' Clear flag if set                 \n" \
      "*                                                  \n" \
      " SYSSTATE POP    Restore SYSSTATE                  \n" \
      "*                                                    " \
      :                                                       \
      : "r"(time)                                             \
      : "r0", "r1", "r2", "r14", "r15");
#else
#define STIMER_WAIT(time)
#endif // __IBM_METAL__

#if defined(__IBM_METAL__)
#define STIMERM(time, routine, id, plist)                     \
  __asm(                                                      \
      "*                                                  \n" \
      " TAM   ,         AMODE64??                         \n" \
      " JM    *+4+4+2   No, skip switching                \n" \
      " OILH  2,X'8000' Set AMODE31 flag                  \n" \
      " SAM31 ,         Set AMODE31                       \n" \
      "*                                                  \n" \
      " SYSSTATE PUSH       Save SYSSTATE                 \n" \
      " SYSSTATE AMODE64=NO                               \n" \
      "*                                                  \n" \
      " STIMERM SET,"                                         \
      "BINTVL=(%1),"                                          \
      "EXIT=(%2),"                                            \
      "ID=%0,"                                                \
      "MF=(E,%3)           \n"                                \
      "*                                                  \n" \
      " TMLH  2,X'8000' Did we switch AMODE??             \n" \
      " JNO   *+4+2     No, skip restore                  \n" \
      " SAM64 ,         Set AMODE64                       \n" \
      "*                                                  \n" \
      " NILH  2,X'7FFF' Clear flag if set                 \n" \
      "*                                                  \n" \
      " SYSSTATE POP    Restore SYSSTATE                  \n" \
      "*                                                    " \
      : "=m"(*id)                                             \
      : "r"(time), "r"(routine), "m"(plist)                   \
      : "r0", "r1", "r2", "r14", "r15");
#else
#define STIMERM(time, routine, id, plist)
#endif // __IBM_METAL__

#if defined(__IBM_METAL__)
#define ECB_WAIT(ecb)                                         \
  __asm(                                                      \
      "*                                                  \n" \
      " WAIT ECB=%0                                       \n" \
      "*                                                    " \
      : "+m"(*ecb)                                            \
      :                                                       \
      : "r0", "r1", "r14", "r15");
#else
#define ECB_WAIT(ecb)
#endif // __IBM_METAL__

#if defined(__IBM_METAL__)
#define ECBS_WAIT(count, list)                                \
  __asm(                                                      \
      "*                                                  \n" \
      " WAIT (%1),ECBLIST=%0                              \n" \
      "*                                                    " \
      : "+m"(*list)                                           \
      : "r"(count)                                            \
      : "r0", "r1", "r14", "r15");
#else
#define ECBS_WAIT(count, list)
#endif // __IBM_METAL__

static void ecbWait(ECB *PTR32 ecb)
{

  ECB_WAIT(ecb);

  return;
}

static void ecbsWait(
    int events, volatile ECB *PTR32 ecbList[],
    int ecbListCount)
{

  union overEcb
  {
    volatile ECB *__ptr32 ecb;
    unsigned int word;
  } oEcb = {0};

  if (ecbListCount >= 1)
  {

    oEcb.ecb = ecbList[ecbListCount - 1];
    oEcb.word |= 0x80000000;

    ecbList[ecbListCount - 1] = oEcb.ecb;

    ECBS_WAIT(events, ecbList);

    oEcb.word &= ~(0x80000000);
    ecbList[ecbListCount - 1] = oEcb.ecb;
  }

  return;
}

static void ecbsWaitOnOne(
    volatile ECB *__ptr32 ecbList[],
    int ecbListCount)
{

  ecbsWait(1, ecbList, ecbListCount);

  return;
}

static void timeWait(int interval)
{

  STIMER_WAIT(&interval);

  return;
}

static TIMER_ID timeExit(int interval, TIMER_EXIT exit)
{

  TIMER_ID id = {0};
  STIMERM_MODEL(dsaStimermModel); // stack var
  STIMERM(&interval, exit, &id, dsaStimermModel);

  return id;
}

#endif //ECBWAIT_H
