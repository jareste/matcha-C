import { Link } from 'react-router'

function Register() {
  return (
    <section className="mx-auto w-full max-w-sm">
      <h1 className="text-2xl font-semibold sm:text-3xl">Sign up</h1>
      <p className="mt-2 text-gray-600 dark:text-gray-400">The registration form is coming soon.</p>
      <p className="mt-6 text-sm">
        Already have an account?{' '}
        <Link to="/login" className="font-medium text-brand hover:underline">
          Log in
        </Link>
      </p>
    </section>
  )
}

export default Register
