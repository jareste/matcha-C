import { Link } from 'react-router'

function Login() {
  return (
    <section className="mx-auto w-full max-w-sm">
      <h1 className="text-2xl font-semibold sm:text-3xl">Log in</h1>
      <p className="mt-2 text-gray-600 dark:text-gray-400">The login form is coming soon.</p>
      <p className="mt-6 text-sm">
        No account yet?{' '}
        <Link to="/register" className="font-medium text-brand hover:underline">
          Sign up
        </Link>
      </p>
    </section>
  )
}

export default Login
