import { useState, type FormEvent } from 'react'
import { Link, useSearchParams } from 'react-router'
import { newPasswordError, resetPassword, type ResetOutcome } from '../lib/passwordReset.ts'
import { PASSWORD_RULES } from '../lib/register.ts'

const linkClass = 'font-medium text-brand hover:underline'
const buttonClass =
  'inline-block rounded-md bg-brand px-4 py-2 text-sm font-medium text-white hover:bg-brand-dark disabled:opacity-60'

function inputClass(invalid: boolean) {
  return [
    'mt-1 block w-full rounded-md border bg-white px-3 py-2 text-sm focus:outline-none focus:ring-1 dark:bg-gray-900',
    invalid
      ? 'border-red-500 focus:border-red-500 focus:ring-red-500'
      : 'border-gray-300 focus:border-brand focus:ring-brand dark:border-gray-700',
  ].join(' ')
}

function ResetPassword() {
  const token = useSearchParams()[0].get('token')?.trim() ?? ''
  const [password, setPassword] = useState('')
  const [confirm, setConfirm] = useState('')
  const [errors, setErrors] = useState<{ password?: string; confirm?: string }>({})
  const [formError, setFormError] = useState('')
  const [submitting, setSubmitting] = useState(false)
  // Final outcomes replace the form; 'weak' and 'error' are shown inside it so the user can retry.
  const [outcome, setOutcome] = useState<ResetOutcome | null>(token ? null : { kind: 'invalid' })

  async function handleSubmit(e: FormEvent<HTMLFormElement>) {
    e.preventDefault()
    setFormError('')

    const clientErrors = newPasswordError(password, confirm)
    setErrors(clientErrors)
    if (clientErrors.password || clientErrors.confirm) return

    setSubmitting(true)
    const res = await resetPassword(token, password)
    setSubmitting(false)

    if (res.kind === 'weak') setErrors({ password: res.message })
    else if (res.kind === 'error') setFormError(res.message)
    else setOutcome(res)
  }

  if (outcome?.kind === 'success') {
    return (
      <section className="mx-auto w-full max-w-sm">
        <h1 className="text-2xl font-semibold sm:text-3xl">Password updated</h1>
        <p role="status" className="mt-2 text-gray-600 dark:text-gray-400">
          Your password has been changed. Use it to log in; the previous one no longer works.
        </p>
        <p className="mt-6">
          <Link to="/login" className={buttonClass}>
            Log in
          </Link>
        </p>
      </section>
    )
  }

  if (outcome?.kind === 'expired' || outcome?.kind === 'invalid') {
    return (
      <section className="mx-auto w-full max-w-sm">
        <h1 className="text-2xl font-semibold sm:text-3xl">
          {outcome.kind === 'expired' ? 'Link expired' : 'Invalid link'}
        </h1>
        <p role="alert" className="mt-2 text-gray-600 dark:text-gray-400">
          {outcome.kind === 'expired'
            ? 'This password reset link has expired. Request a new one to continue.'
            : 'This password reset link is invalid or has already been used. Check that you opened the full link from the email, or request a new one.'}
        </p>
        <p className="mt-6 flex gap-4 text-sm">
          <Link to="/forgot-password" className={linkClass}>
            Request a new link
          </Link>
          <Link to="/login" className={linkClass}>
            Log in
          </Link>
        </p>
      </section>
    )
  }

  return (
    <section className="mx-auto w-full max-w-sm">
      <h1 className="text-2xl font-semibold sm:text-3xl">Choose a new password</h1>

      <form className="mt-6 space-y-4" noValidate onSubmit={handleSubmit}>
        {formError && (
          <p role="alert" className="rounded-md bg-red-50 px-3 py-2 text-sm text-red-700 dark:bg-red-950 dark:text-red-300">
            {formError}
          </p>
        )}

        <div>
          <label htmlFor="password" className="block text-sm font-medium">
            New password
          </label>
          <input
            id="password"
            name="password"
            type="password"
            autoComplete="new-password"
            value={password}
            onChange={(e) => {
              setPassword(e.target.value)
              setErrors((errs) => ({ ...errs, password: undefined }))
            }}
            aria-invalid={!!errors.password}
            aria-describedby={[errors.password && 'password-error', 'password-rules'].filter(Boolean).join(' ')}
            className={inputClass(!!errors.password)}
          />
          {errors.password && (
            <p id="password-error" className="mt-1 text-sm text-red-600 dark:text-red-400">
              {errors.password}
            </p>
          )}
          <ul id="password-rules" aria-label="Password rules" className="mt-2 space-y-0.5 text-xs">
            {PASSWORD_RULES.map((rule) => {
              const met = password !== '' && rule.test(password, '')
              return (
                <li
                  key={rule.id}
                  data-met={met}
                  className={met ? 'text-green-700 dark:text-green-400' : 'text-gray-500 dark:text-gray-400'}
                >
                  {met ? '✓' : '•'} {rule.label}
                </li>
              )
            })}
          </ul>
        </div>

        <div>
          <label htmlFor="confirm_password" className="block text-sm font-medium">
            Confirm new password
          </label>
          <input
            id="confirm_password"
            name="confirm_password"
            type="password"
            autoComplete="new-password"
            value={confirm}
            onChange={(e) => {
              setConfirm(e.target.value)
              setErrors((errs) => ({ ...errs, confirm: undefined }))
            }}
            aria-invalid={!!errors.confirm}
            aria-describedby={errors.confirm ? 'confirm_password-error' : undefined}
            className={inputClass(!!errors.confirm)}
          />
          {errors.confirm && (
            <p id="confirm_password-error" className="mt-1 text-sm text-red-600 dark:text-red-400">
              {errors.confirm}
            </p>
          )}
        </div>

        <button type="submit" disabled={submitting} className={`w-full ${buttonClass}`}>
          {submitting ? 'Updating…' : 'Update password'}
        </button>
      </form>

      <p className="mt-6 text-sm">
        <Link to="/login" className={linkClass}>
          Back to log in
        </Link>
      </p>
    </section>
  )
}

export default ResetPassword
