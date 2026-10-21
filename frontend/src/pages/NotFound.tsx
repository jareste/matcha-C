import { Link } from 'react-router'

function NotFound() {
  return (
    <section className="flex flex-1 flex-col items-center justify-center text-center">
      <p className="text-5xl font-bold text-brand">404</p>
      <h1 className="mt-4 text-2xl font-semibold">Page not found</h1>
      <p className="mt-2 text-gray-600 dark:text-gray-400">This page doesn't exist.</p>
      <Link
        to="/"
        className="mt-6 rounded-md bg-brand px-4 py-2 text-sm font-medium text-white hover:bg-brand-dark"
      >
        Back to home
      </Link>
    </section>
  )
}

export default NotFound
