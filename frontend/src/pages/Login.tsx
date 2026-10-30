import { useState, type ChangeEvent, type FormEvent } from 'react'
import { Link, Navigate, useLocation } from 'react-router'
import { useAuth } from '../lib/authContext.ts'

type LoginField = 'email' | 'password'
type LoginForm = Record<LoginField, string>
type FieldErrors = Partial<Record<LoginField, string>>

const FIELDS: { name: LoginField; label: string; type: string; autoComplete: string }[] = [
  { name: 'email', label: 'Email', type: 'email', autoComplete: 'email' },
  { name: 'password', label: 'Password', type: 'password', autoComplete: 'current-password' },
]

// Backend messages that concern a single field; the rest stay form-level.
const SERVER_FIELD_ERRORS: Record<string, LoginField> = {
  'Email is required': 'email',
  'Password is required': 'password',
}

function inputClass(invalid: boolean) {
  return [
    'mt-1 block w-full rounded-md border bg-white px-3 py-2 text-sm focus:outline-none focus:ring-1 dark:bg-gray-900',
    invalid
      ? 'border-red-500 focus:border-red-500 focus:ring-red-500'
      : 'border-gray-300 focus:border-brand focus:ring-brand dark:border-gray-700',
  ].join(' ')
}

function Login() {
  const { status, login } = useAuth()
  const from = (useLocation().state as { from?: string } | null)?.from ?? '/'
  const [form, setForm] = useState<LoginForm>({ email: '', password: '' })
  const [errors, setErrors] = useState<FieldErrors>({})
  const [formError, setFormError] = useState('')
  const [submitting, setSubmitting] = useState(false)

  // Covers both a fresh login and a user who opens /login while already logged in.
  if (status === 'authenticated') return <Navigate to={from} replace />

  function handleChange(e: ChangeEvent<HTMLInputElement>) {
    const name = e.target.name as LoginField
    setForm((f) => ({ ...f, [name]: e.target.value }))
    setErrors((errs) => ({ ...errs, [name]: undefined }))
  }

  async function handleSubmit(e: FormEvent<HTMLFormElement>) {
    e.preventDefault()
    setFormError('')

    const email = form.email.trim()
    const clientErrors: FieldErrors = {}
    if (!email) clientErrors.email = 'Email is required'
    if (!form.password) clientErrors.password = 'Password is required'
    setErrors(clientErrors)
    if (Object.keys(clientErrors).length) return

    setSubmitting(true)
    const res = await login(email, form.password)
    if (res.ok) return
    setSubmitting(false)

    const field = SERVER_FIELD_ERRORS[res.error]
    if (field) setErrors({ [field]: res.error })
    else setFormError(res.error)
  }

  return (
    <section className="mx-auto w-full max-w-sm">
      <h1 className="text-2xl font-semibold sm:text-3xl">Log in</h1>

      <form className="mt-6 space-y-4" noValidate onSubmit={handleSubmit}>
        {formError && (
          <p role="alert" className="rounded-md bg-red-50 px-3 py-2 text-sm text-red-700 dark:bg-red-950 dark:text-red-300">
            {formError}
          </p>
        )}

        {FIELDS.map(({ name, label, type, autoComplete }) => {
          const error = errors[name]
          return (
            <div key={name}>
              <label htmlFor={name} className="block text-sm font-medium">
                {label}
              </label>
              <input
                id={name}
                name={name}
                type={type}
                autoComplete={autoComplete}
                value={form[name]}
                onChange={handleChange}
                aria-invalid={!!error}
                aria-describedby={error ? `${name}-error` : undefined}
                className={inputClass(!!error)}
              />
              {error && (
                <p id={`${name}-error`} className="mt-1 text-sm text-red-600 dark:text-red-400">
                  {error}
                </p>
              )}
            </div>
          )
        })}

        <button
          type="submit"
          disabled={submitting}
          className="w-full rounded-md bg-brand px-4 py-2 text-sm font-medium text-white hover:bg-brand-dark disabled:opacity-60"
        >
          {submitting ? 'Logging in…' : 'Log in'}
        </button>
      </form>

      <p className="mt-6 text-sm">
        <Link to="/forgot-password" className="font-medium text-brand hover:underline">
          Forgot your password?
        </Link>
      </p>
      <p className="mt-2 text-sm">
        No account yet?{' '}
        <Link to="/register" className="font-medium text-brand hover:underline">
          Sign up
        </Link>
      </p>
    </section>
  )
}

export default Login
