/* danpType.h - one line definition */

/* All Rights Reserved */

#ifndef INC_DANP_TYPES_H
#define INC_DANP_TYPES_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes */


/* Configurations */


/* Definitions */

#ifndef WEAK
/** @brief Weak attribute for overridable functions. */
#define WEAK __attribute__((weak))
#endif

#ifndef PACKED
/** @brief Packed attribute for structures. */
#define PACKED __attribute__((packed))
#endif

#ifndef UNUSED
/** @brief Macro to mark unused variables and avoid compiler warnings. */
#define UNUSED(x) (void)(x)
#endif

/* Types */


/* External Declarations */


#ifdef __cplusplus
}
#endif

#endif /* INC_DANP_TYPES_H */
