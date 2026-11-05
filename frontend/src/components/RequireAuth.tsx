import { Navigate, Outlet, useLocation } from 'react-router'
import { useAuth } from '../lib/authContext.ts'

// Guards private routes. Anonymous users (never logged in, logged out, or whose
// session just expired) go to /login, which sends them back to the same URL,
// query string included, once they are logged in.
function RequireAuth() {
  const { status } = useAuth()
  const location = useLocation()

  if (status === 'loading') {
    return (
      <p role="status" className="text-gray-600 dark:text-gray-400">
        Loading…
      </p>
    )
  }
  if (status === 'anonymous') {
    const from = location.pathname + location.search + location.hash
    return <Navigate to="/login" replace state={{ from }} />
  }
  return <Outlet />
}

export default RequireAuth
