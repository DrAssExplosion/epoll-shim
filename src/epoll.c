#include <errno.h>
#include <limits.h>
#include "up.h"

#include <sys/epoll.h>
#include "upoll.h"

static upoll_t* ups[OPEN_MAX];
static int epoll_index = 1;

int
epoll_create(int size)
{
        if (epoll_index >= OPEN_MAX) {
                errno = ENFILE;
                return -1;
        }
        ups[epoll_index++] = upoll_create(size);
        return epoll_index - 1;
}

int
epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)
{
        upoll_t* upq = ups[epfd];
        upoll_event_t* uevent = (upoll_event_t*) event;
        return upoll_ctl(upq, op, fd, uevent);
}

int
epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout)
{
        upoll_t* upq = ups[epfd];
        upoll_event_t* uevents = (upoll_event_t*) events;
        return upoll_wait(upq, uevents, maxevents, timeout);
}


int
epoll_pwait(int fd, struct epoll_event *ev, int cnt, int to,
    sigset_t const *sigs)
{
	return epoll_wait(fd, ev, cnt, to);
}


int
epoll_create1(int flags)
{
	return epoll_create(1);
}


int
epoll_shim_close(int fd)
{
	//FDContextMapNode *node;
//
	//node = epoll_shim_ctx_remove_node(&epoll_shim_ctx, fd);
	//if (!node) {
	//	return close(fd);
	//}
//
	//errno_t ec = fd_context_map_node_destroy(node);
	//if (ec != 0) {
	//	errno = ec;
	//	return -1;
	//}

	return 0;
}

// #include <sys/epoll.h>

// #include <sys/event.h>
// #include <sys/param.h>
// #include <sys/stat.h>
// #include <sys/time.h>

// #include <assert.h>
// #include <errno.h>
// #include <poll.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <time.h>

// #include "epoll_shim_ctx.h"

// #ifdef __NetBSD__
// #define ppoll pollts
// #elif defined(__APPLE__)
// int
// ppoll(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout_ts, const sigset_t *sigmask)
// {
// 	const int timeout = (timeout_ts == NULL)
// 		? -1
// 		: (int)(timeout_ts->tv_sec * 1000 + timeout_ts->tv_nsec / 1000000);
// 	sigset_t origmask;
// 	sigprocmask(SIG_SETMASK, sigmask, &origmask);
// 	int ready = poll(fds, nfds, timeout);
// 	sigprocmask(SIG_SETMASK, &origmask, NULL);
// 	return ready;
// }
// #endif

// // TODO(jan): Remove this once the definition is exposed in <sys/time.h> in
// // all supported FreeBSD versions.
// #ifndef timespecsub
// #define timespecsub(tsp, usp, vsp)                                            \
// 	do {                                                                  \
// 		(vsp)->tv_sec = (tsp)->tv_sec - (usp)->tv_sec;                \
// 		(vsp)->tv_nsec = (tsp)->tv_nsec - (usp)->tv_nsec;             \
// 		if ((vsp)->tv_nsec < 0) {                                     \
// 			(vsp)->tv_sec--;                                      \
// 			(vsp)->tv_nsec += 1000000000L;                        \
// 		}                                                             \
// 	} while (0)
// #endif

// static errno_t
// epollfd_close(FDContextMapNode *node)
// {
// 	return epollfd_ctx_terminate(&node->ctx.epollfd);
// }

// static FDContextVTable const epollfd_vtable = {
//     .read_fun = fd_context_default_read,
//     .write_fun = fd_context_default_write,
//     .close_fun = epollfd_close,
// };

// static FDContextMapNode *
// epoll_create_impl(errno_t *ec)
// {
// 	FDContextMapNode *node;

// 	node = epoll_shim_ctx_create_node(&epoll_shim_ctx, ec);
// 	if (!node) {
// 		return NULL;
// 	}

// 	node->flags = 0;

// 	if ((*ec = epollfd_ctx_init(&node->ctx.epollfd, /**/
// 		 node->fd)) != 0) {
// 		goto fail;
// 	}

// 	node->vtable = &epollfd_vtable;
// 	return node;

// fail:
// 	epoll_shim_ctx_remove_node_explicit(&epoll_shim_ctx, node);
// 	(void)fd_context_map_node_destroy(node);
// 	return NULL;
// }

// static int
// epoll_create_common(void)
// {
// 	FDContextMapNode *node;
// 	errno_t ec;

// 	node = epoll_create_impl(&ec);
// 	if (!node) {
// 		errno = ec;
// 		return -1;
// 	}

// 	return node->fd;
// }

// int
// epoll_create(int size)
// {
// 	if (size <= 0) {
// 		errno = EINVAL;
// 		return -1;
// 	}

// 	return epoll_create_common();
// }

// int
// epoll_create1(int flags)
// {
// 	if (flags & ~EPOLL_CLOEXEC) {
// 		errno = EINVAL;
// 		return -1;
// 	}

// 	return epoll_create_common();
// }

// static errno_t
// epoll_ctl_impl(int fd, int op, int fd2, struct epoll_event *ev)
// {
// 	if (!ev && op != EPOLL_CTL_DEL) {
// 		return EFAULT;
// 	}

// 	FDContextMapNode *node = epoll_shim_ctx_find_node(&epoll_shim_ctx, fd);
// 	if (!node || node->vtable != &epollfd_vtable) {
// 		struct stat sb;
// 		return (fd < 0 || fstat(fd, &sb) < 0) ? EBADF : EINVAL;
// 	}

// 	return epollfd_ctx_ctl(&node->ctx.epollfd, op, fd2, ev);
// }

// int
// epoll_ctl(int fd, int op, int fd2, struct epoll_event *ev)
// {
// 	errno_t ec = epoll_ctl_impl(fd, op, fd2, ev);
// 	if (ec != 0) {
// 		errno = ec;
// 		return -1;
// 	}

// 	return 0;
// }

// static bool
// is_no_wait_deadline(struct timespec const *deadline)
// {
// 	return (deadline && deadline->tv_sec == 0 && deadline->tv_nsec == 0);
// }

// static errno_t
// epollfd_ctx_wait_or_block(EpollFDCtx *epollfd, struct epoll_event *ev, int cnt,
//     int *actual_cnt, struct timespec const *deadline, sigset_t const *sigs)
// {
// 	errno_t ec;

// 	for (;;) {
// 		if ((ec = epollfd_ctx_wait(epollfd, /**/
// 			 ev, cnt, actual_cnt)) != 0) {
// 			return ec;
// 		}

// 		if (*actual_cnt || is_no_wait_deadline(deadline)) {
// 			return 0;
// 		}

// 		struct timespec timeout;

// 		if (deadline) {
// 			struct timespec current_time;

// 			if (clock_gettime(CLOCK_MONOTONIC, /**/
// 				&current_time) < 0) {
// 				return errno;
// 			}

// 			timespecsub(deadline, &current_time, &timeout);
// 			if (timeout.tv_sec < 0 ||
// 			    is_no_wait_deadline(&timeout)) {
// 				return 0;
// 			}
// 		}

// 		(void)pthread_mutex_lock(&epollfd->mutex);

// 		nfds_t nfds = (nfds_t)(1 + epollfd->poll_fds_size);

// 		size_t size;
// 		if (__builtin_mul_overflow(nfds, sizeof(struct pollfd),
// 			&size)) {
// 			ec = ENOMEM;
// 			(void)pthread_mutex_unlock(&epollfd->mutex);
// 			return ec;
// 		}

// 		struct pollfd *pfds = malloc(size);
// 		if (!pfds) {
// 			ec = errno;
// 			(void)pthread_mutex_unlock(&epollfd->mutex);
// 			return ec;
// 		}

// 		epollfd_ctx_fill_pollfds(epollfd, pfds);

// 		(void)pthread_mutex_lock(&epollfd->nr_polling_threads_mutex);
// 		++epollfd->nr_polling_threads;
// 		(void)pthread_mutex_unlock(&epollfd->nr_polling_threads_mutex);

// 		(void)pthread_mutex_unlock(&epollfd->mutex);

// 		/*
// 		 * This surfaced a race condition when
// 		 * registering/unregistering poll-only fds. The tests should
// 		 * still succeed if this is enabled.
// 		 */
// #if 0
// 		usleep(500000);
// #endif

// 		int n = ppoll(pfds, nfds, deadline ? &timeout : NULL, sigs);
// 		if (n < 0) {
// 			ec = errno;
// 		}

// 		free(pfds);

// 		(void)pthread_mutex_lock(&epollfd->nr_polling_threads_mutex);
// 		--epollfd->nr_polling_threads;
// 		if (epollfd->nr_polling_threads == 0) {
// 			(void)pthread_cond_signal(
// 			    &epollfd->nr_polling_threads_cond);
// 		}
// 		(void)pthread_mutex_unlock(&epollfd->nr_polling_threads_mutex);

// 		if (n < 0) {
// 			return ec;
// 		}
// 	}
// }

// static errno_t
// timeout_to_deadline(struct timespec *deadline, int to)
// {
// 	assert(to >= 0);

// 	if (to == 0) {
// 		*deadline = (struct timespec){0, 0};
// 	} else if (to > 0) {
// 		if (clock_gettime(CLOCK_MONOTONIC, deadline) < 0) {
// 			return errno;
// 		}

// 		if (__builtin_add_overflow(deadline->tv_sec, to / 1000 + 1,
// 			&deadline->tv_sec)) {
// 			return EINVAL;
// 		}
// 		deadline->tv_sec -= 1;

// 		deadline->tv_nsec += (to % 1000) * 1000000L;
// 		if (deadline->tv_nsec >= 1000000000) {
// 			deadline->tv_nsec -= 1000000000;
// 			deadline->tv_sec += 1;
// 		}
// 	}

// 	return 0;
// }

// static errno_t
// epoll_pwait_impl(int fd, struct epoll_event *ev, int cnt, int to,
//     sigset_t const *sigs, int *actual_cnt)
// {
// 	if (cnt < 1 || cnt > (int)(INT_MAX / sizeof(struct epoll_event))) {
// 		return EINVAL;
// 	}

// 	FDContextMapNode *node = epoll_shim_ctx_find_node(&epoll_shim_ctx, fd);
// 	if (!node || node->vtable != &epollfd_vtable) {
// 		struct stat sb;
// 		return (fd < 0 || fstat(fd, &sb) < 0) ? EBADF : EINVAL;
// 	}

// 	struct timespec deadline;
// 	errno_t ec;
// 	if (to >= 0 && (ec = timeout_to_deadline(&deadline, to)) != 0) {
// 		return ec;
// 	}

// 	return epollfd_ctx_wait_or_block(&node->ctx.epollfd, ev, cnt,
// 	    actual_cnt, (to >= 0) ? &deadline : NULL, sigs);
// }

// int
// epoll_pwait(int fd, struct epoll_event *ev, int cnt, int to,
//     sigset_t const *sigs)
// {
// 	int actual_cnt;

// 	errno_t ec = epoll_pwait_impl(fd, ev, cnt, to, sigs, &actual_cnt);
// 	if (ec != 0) {
// 		errno = ec;
// 		return -1;
// 	}

// 	return actual_cnt;
// }

// int
// epoll_wait(int fd, struct epoll_event *ev, int cnt, int to)
// {
// 	return epoll_pwait(fd, ev, cnt, to, NULL);
// }
