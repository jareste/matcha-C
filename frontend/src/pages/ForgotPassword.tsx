import { useState, type FormEvent } from 'react'
import { Link } from 'react-router'
import { requestPasswordReset } from '../lib/passwordReset.ts'

const EMAIL_RE = /^[^\s@]+@[^\s@]+\.[^\s@]+$/

function inputClass(invalid: boolean) {
  return [
    'mt-1 block w-full rounded-md border bg-white px-3 py-2 text-sm focus:outline-none focus:ring-1 dark:bg-gray-900',
    invalid
      ? 'border-red-500 focus:border-red-500 focus:ring-red-500'
      : 'border-gray-300 focus:border-brand focus:ring-brand dark:border-gray-700',
  ].join(' ')
}

function ForgotPassword() {
  const [email, setEmail] = useState('')
  const [fieldError, setFieldError] = useState('')
  const [formError, setFormError] = useState('')
  const [submitting, setSubmitting] = useState(false)
  const [sent, setSent] = useState(false)

  async function handleSubmit(e: FormEvent<HTMLFormElement>) {
    e.preventDefault()
    setFormError('')

    const value = email.trim()
    if (!value) return setFieldError('Email is required')
    if (!EMAIL_RE.test(value)) return setFieldError('Enter a valid email address')
    setFieldError('')

    setSubmitting(true)
    const outcome = await requestPasswordReset(value)
    setSubmitting(false)

    // The confirmation is identical whether or not the email is registered.
    if (outcome.kind === 'sent') setSent(true)
    else setFormError(outcome.message)
  }

  if (sent) {
    return (
      <section className="mx-auto w-full max-w-sm">
        <h1 className="text-2xl font-semibold sm:text-3xl">Check your email</h1>
        <p role="status" className="mt-2 text-gray-600 dark:text-gray-400">
          If an account exists for <strong className="text-gray-900 dark:text-gray-100">{email.trim()}</strong>, we
          sent it a link to reset the password. The link expires after a short while. Check your spam folder if you
          do not see it.
        </p>
        <p className="mt-6 text-sm">
          <Link to="/login" className="font-medium text-brand hover:underline">
            Back to log in
          </Link>
        </p>
      </section>
    )
  }

  return (
    <section className="mx-auto w-full max-w-sm">
      <h1 className="text-2xl font-semibold sm:text-3xl">Forgot your password?</h1>
      <p className="mt-2 text-gray-600 dark:text-gray-400">
        Enter the email of your account and we will send you a link to choose a new password.
      </p>

      <form className="mt-6 space-y-4" noValidate onSubmit={handleSubmit}>
        {formError && (
          <p role="alert" className="rounded-md bg-red-50 px-3 py-2 text-sm text-red-700 dark:bg-red-950 dark:text-red-300">
            {formError}
          </p>
        )}

        <div>
          <label htmlFor="email" className="block text-sm font-medium">
            Email
          </label>
          <input
            id="email"
            name="email"
            type="email"
            autoComplete="email"
            value={email}
            onChange={(e) => {
              setEmail(e.target.value)
              setFieldError('')
            }}
            aria-invalid={!!fieldError}
            aria-describedby={fieldError ? 'email-error' : undefined}
            className={inputClass(!!fieldError)}
          />
          {fieldError && (
            <p id="email-error" className="mt-1 text-sm text-red-600 dark:text-red-400">
              {fieldError}
            </p>
          )}
        </div>

        <button
          type="submit"
          disabled={submitting}
          className="w-full rounded-md bg-brand px-4 py-2 text-sm font-medium text-white hover:bg-brand-dark disabled:opacity-60"
        >
          {submitting ? 'Sending…' : 'Send reset link'}
        </button>
      </form>

      <p className="mt-6 text-sm">
        <Link to="/login" className="font-medium text-brand hover:underline">
          Back to log in
        </Link>
      </p>
    </section>
  )
}

export default ForgotPassword
