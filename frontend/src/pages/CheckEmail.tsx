import { Link, useLocation } from 'react-router'

function CheckEmail() {
  const email = (useLocation().state as { email?: string } | null)?.email

  return (
    <section className="mx-auto w-full max-w-sm">
      <h1 className="text-2xl font-semibold sm:text-3xl">Check your email</h1>
      <p className="mt-2 text-gray-600 dark:text-gray-400">
        Your account has been created. We sent a confirmation link to{' '}
        {email ? <strong className="text-gray-900 dark:text-gray-100">{email}</strong> : 'your email address'}. Open it to
        activate your account.
      </p>
      <p className="mt-6 text-sm">
        <Link to="/login" className="font-medium text-brand hover:underline">
          Go to log in
        </Link>
      </p>
    </section>
  )
}

export default CheckEmail
