import { useState, type ChangeEvent, type FormEvent } from 'react'
import { Link, useNavigate } from 'react-router'
import { apiPost } from '../lib/api.ts'
import {
  PASSWORD_RULES,
  serverErrorField,
  toPayload,
  validateRegister,
  type FieldErrors,
  type RegisterField,
  type RegisterForm,
} from '../lib/register.ts'

const EMPTY_FORM: RegisterForm = {
  username: '',
  email: '',
  first_name: '',
  last_name: '',
  password: '',
  confirm_password: '',
}

const FIELDS: { name: RegisterField; label: string; type: string; autoComplete: string }[] = [
  { name: 'username', label: 'Username', type: 'text', autoComplete: 'username' },
  { name: 'email', label: 'Email', type: 'email', autoComplete: 'email' },
  { name: 'first_name', label: 'First name', type: 'text', autoComplete: 'given-name' },
  { name: 'last_name', label: 'Last name', type: 'text', autoComplete: 'family-name' },
  { name: 'password', label: 'Password', type: 'password', autoComplete: 'new-password' },
  { name: 'confirm_password', label: 'Confirm password', type: 'password', autoComplete: 'new-password' },
]

function inputClass(invalid: boolean) {
  return [
    'mt-1 block w-full rounded-md border bg-white px-3 py-2 text-sm focus:outline-none focus:ring-1 dark:bg-gray-900',
    invalid
      ? 'border-red-500 focus:border-red-500 focus:ring-red-500'
      : 'border-gray-300 focus:border-brand focus:ring-brand dark:border-gray-700',
  ].join(' ')
}

function Register() {
  const navigate = useNavigate()
  const [form, setForm] = useState<RegisterForm>(EMPTY_FORM)
  const [errors, setErrors] = useState<FieldErrors>({})
  const [formError, setFormError] = useState('')
  const [submitting, setSubmitting] = useState(false)

  function handleChange(e: ChangeEvent<HTMLInputElement>) {
    const name = e.target.name as RegisterField
    setForm((f) => ({ ...f, [name]: e.target.value }))
    // Drop the stale error of the field being edited; the rest stays until next submit.
    setErrors((errs) => ({ ...errs, [name]: undefined }))
  }

  async function handleSubmit(e: FormEvent<HTMLFormElement>) {
    e.preventDefault()
    setFormError('')

    const clientErrors = validateRegister(form)
    setErrors(clientErrors)
    if (Object.keys(clientErrors).length) return

    setSubmitting(true)
    const payload = toPayload(form)
    const res = await apiPost('/api/register', payload)
    setSubmitting(false)

    if (res.ok) {
      navigate('/register/check-email', { state: { email: payload.email } })
      return
    }
    const field = serverErrorField(res.error)
    if (field) setErrors({ [field]: res.error })
    else setFormError(res.error)
  }

  return (
    <section className="mx-auto w-full max-w-sm">
      <h1 className="text-2xl font-semibold sm:text-3xl">Sign up</h1>

      <form className="mt-6 space-y-4" noValidate onSubmit={handleSubmit}>
        {formError && (
          <p role="alert" className="rounded-md bg-red-50 px-3 py-2 text-sm text-red-700 dark:bg-red-950 dark:text-red-300">
            {formError}
          </p>
        )}

        {FIELDS.map(({ name, label, type, autoComplete }) => {
          const error = errors[name]
          const describedBy = [error && `${name}-error`, name === 'password' && 'password-rules'].filter(Boolean).join(' ')
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
                aria-describedby={describedBy || undefined}
                className={inputClass(!!error)}
              />
              {error && (
                <p id={`${name}-error`} className="mt-1 text-sm text-red-600 dark:text-red-400">
                  {error}
                </p>
              )}
              {name === 'password' && (
                <ul id="password-rules" aria-label="Password rules" className="mt-2 space-y-0.5 text-xs">
                  {PASSWORD_RULES.map((rule) => {
                    const met = form.password !== '' && rule.test(form.password, form.username)
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
              )}
            </div>
          )
        })}

        <button
          type="submit"
          disabled={submitting}
          className="w-full rounded-md bg-brand px-4 py-2 text-sm font-medium text-white hover:bg-brand-dark disabled:opacity-60"
        >
          {submitting ? 'Creating account…' : 'Create account'}
        </button>
      </form>

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
