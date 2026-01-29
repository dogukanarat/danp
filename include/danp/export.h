/* export.h - one line definition */

/* All Rights Reserved */

#ifndef INC_DANP_EXPORT_H
#define INC_DANP_EXPORT_H

/* Includes */


#ifdef __cplusplus
extern "C" {
#endif


/* Configurations */


/* Definitions */

#if defined(_WIN32) || defined(__CYGWIN__)
#ifdef DANP_EXPORTS
#define DANP_API __declspec(dllexport)
#else
#define DANP_API __declspec(dllimport)
#endif
#elif defined(__GNUC__) && __GNUC__ >= 4
#define DANP_API __attribute__((visibility("default")))
#else
#define DANP_API
#endif

/* Types */


/* External Declarations */


#ifdef __cplusplus
}
#endif

#endif /* INC_DANP_EXPORT_H */