#ifndef IX_EXCEPT_H_
#define IX_EXCEPT_H_

/*
 * Annotation for unreachable code.
 *
 * If this is executed, it is equivalent to "assert(0); exit(1);".
 */
void ix_notreached(void);

#endif  /* IX_EXCEPT_H_ */
