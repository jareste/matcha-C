import { Navigate, Outlet, useLocation } from 'react-router'
import { useAuth } from '../lib/authContext.ts'

// Guards private routes. Anonymous users go to /login, which sends them back
// here once they are logged in.
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
  if (status === 'anonymous') return <Navigate to="/login" replace state={{ from: location.pathname }} />
  return <Outlet />
}

export default RequireAuth
